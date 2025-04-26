#include "HTTP-Server.h"

#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "HTTP-Context.h"
#include "HTTP-Request.h"
#include "HTTP-Response.h"
#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "base/util.h"
#include "log/Logger.h"
#include "tcp/Buffer.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"

#define CONNECTION_TIMEOUT_SECOND 900.

namespace fs = std::filesystem;

/* for inotify */
static std::unordered_map<int, fs::path> watch_map;
// <cookie, full_path>
static std::unordered_map<uint32_t, fs::path> move_file_storage;
struct MoveDirStorage {
  int wd = -1;
  uint32_t cookie = 0;
  fs::path old_path;
  fs::path new_path;
};
static std::vector<MoveDirStorage> move_dir_storage;

void add_watches_recursive(int fd, const fs::path& path) {
  int wd = inotify_add_watch(
      fd, path.c_str(),
      IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF | IN_ISDIR | IN_IGNORED);
  if (wd < 0) {
    perror(("watch failed for " + path.string()).c_str());
    return;
  }

  // insert parent path into the map
  watch_map[wd] = path;

  // recursively add watches for subdirs
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_directory()) {
      add_watches_recursive(fd, entry.path());
    }
  }
}

HTTPServer::HTTPServer(EventLoop* loop, const char* ip, const int port, const std::filesystem::path& staticPath)
    : loop_(loop),
      tcpServer_(std::make_unique<TCPServer>(loop, ip, port)),
      fileCache_(std::make_unique<FileLRU>(staticPath)) {
  /**
   * set TCPServer::on_connection_callback_
   */
  tcpServer_->onConnection([this](const std::shared_ptr<TCPConnection>& conn) {
    int fd_clnt = conn->fd();

    // init the HTTPContext
    conn->setContext(new HTTPContext());

    // set timer for timeout closing
    conn->setTimer(loop_->runEvery(CONNECTION_TIMEOUT_SECOND, [this, conn]() {
      if (conn->connectionState() == TCPState::Connected) {
        if (conn->lastActive() + CONNECTION_TIMEOUT_SECOND < TimeStamp::now()) {
          // cacel the timer first
          loop_->canelTimer(conn->timer());
          conn->handleClose();
        }
      } else if (conn->connectionState() == TCPState::Disconnected) {
        // if the connection is already closed, cancel the timer
        loop_->canelTimer(conn->timer());
      } else {
        LOG_WARN << "HTTPServer::HTTPServer, unknown state";
      }
    }));

    // print out peer's info
    struct sockaddr_in addr_peer;
    socklen_t addrlength_peer = sizeof(addr_peer);
    getpeername(fd_clnt, (struct sockaddr*)&addr_peer, &addrlength_peer);
    std::cout << "tid-" << CurrentThread::gettid() << " Connection"
              << "[fd#" << fd_clnt << ']' << " from " << inet_ntoa(addr_peer.sin_addr) << ':'
              << ntohs(addr_peer.sin_port) << std::endl;
  });

  /**
   * set TCPConnection::on_message_callback_
   */
  tcpServer_->onMessage([this](const std::shared_ptr<TCPConnection>& conn) {
    if (TCPState::Connected != conn->connectionState()) {
      LOG_WARN << "HTTPConnection::EnableHTTPConnection, on_message_callback_ called while disconnected";
      return;
    }
    using ParseState = HTTPContext::ParseState;
    HTTPContext* http_context_ = static_cast<HTTPContext*>(conn->context());
    ParseState return_state = http_context_->parseRequest(conn->inBuffer());
    switch (return_state) {
      case ParseState::COMPLETE: {
        HTTPRequest* req = http_context_->getRequest();
        // check if client wish to keep the connection
        std::string conn_state = req->getHeaderByKey("Connection");
        bool is_clnt_want_to_close = (util::toLower(conn_state) == "close");
        HTTPResponse res(is_clnt_want_to_close);
        // set the res according to the request
        on_response_callback_(req, &res);
        if (res.bigFile() < 0) {  // no big file
          conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        } else {  // needs to send big file
          conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
          conn->sendFile(res.bigFile());
        }
        // reset the state of the parser and the snapshot_
        http_context_->reset();
        if (res.isClose()) {  // if the client wish to close connection
          if (conn->connectionState() == TCPState::Disconnected) {
            break;
          }
          conn->handleClose();
        }
        break;
      }
      // handle invalid cases
      case ParseState::INVALID_METHOD: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid HTTP METHOD";
        HTTPResponse res(false, HTTPResponse::Status::NotImplemented);
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        break;
      }
      case ParseState::INVALID_URL: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid URL";
        HTTPResponse res(false, HTTPResponse::Status::NotFound);
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        break;
      }
      case ParseState::INVALID_PROTOCOL: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid PROTOCOL";
        HTTPResponse res(false, HTTPResponse::Status::HTTPVersionNotSupported);
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        break;
      }
      case ParseState::INVALID_HEADER: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid HEADER";
        HTTPResponse res(false, HTTPResponse::Status::Forbidden);
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        break;
      }
      case ParseState::INVALID_CRLF: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid CRLF";
        HTTPResponse res(false, HTTPResponse::Status::BadRequest);
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        break;
      }
      // this one will close the connection
      case ParseState::INVALID: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, just invalid";
        HTTPResponse res(true, HTTPResponse::Status::BadRequest);
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        if (conn->connectionState() == TCPState::Disconnected) {
          break;
        }
        conn->handleClose();
        break;
      }
      // UNFINISHED, waiting for more data
      default: {
        break;
      }
    }
  });

  /**
   * inotify to watch the static file directory
   */
  // create inotify Channel
  int inotify_fd = inotify_init();
  if (inotify_fd < 0) {
    perror("inotify_init");
  }

  // start watching the dir (recursively)
  add_watches_recursive(inotify_fd, staticPath);

  inotifyChannel_ = std::make_unique<Channel>(inotify_fd, loop_, true, false, true, false);
  inotifyChannel_->setReadCallback([this, inotify_fd]() {
    // char buffer[2 * (sizeof(struct inotify_event) + NAME_MAX + 1)];
    char buffer[4096];  // big buffer for robustness
    int length = read(inotify_fd, buffer, sizeof(buffer));
    if (length < 0) {
      perror("read");
      return;
    }

    int i = 0;
    while (i < length) {
      struct inotify_event* event = (struct inotify_event*)&buffer[i];

      fs::path parent_path = watch_map[event->wd];
      fs::path entry_path = (event->len > 0) ? (parent_path / event->name) : parent_path;
      std::cout << "entry_path: " << entry_path << ", event: ";

      if (event->mask & IN_CREATE) {
        if (event->mask & IN_ISDIR) {
          std::cout << "Dir created" << std::endl;
          add_watches_recursive(inotify_fd, entry_path);
        } else {
          std::cout << "File created" << std::endl;
          fileCache_->addFile(entry_path);
        }
      } else if (event->mask & IN_DELETE) {
        if (event->mask & IN_ISDIR) {
          std::cout << "Dir deleted" << std::endl;
          fileCache_->delDir(entry_path);
        } else {
          std::cout << "File deleted" << std::endl;
          fileCache_->delFile(entry_path);
        }
      } else if (event->mask & IN_MODIFY) {
        std::cout << "File modified" << std::endl;
        fileCache_->addFile(entry_path);  // update the file
      } else if (event->mask & IN_MOVED_FROM) {
        if (event->mask & IN_ISDIR) {
          std::cout << "Dir moved from, cookie: " << event->cookie << std::endl;
          // search vector<MoveDirStorage> by cookie to find the corresponding IN_MOVED_TO
          for (auto it = move_dir_storage.begin(); it != move_dir_storage.end(); ++it) {
            if (it->old_path == entry_path) {
              if (move_file_storage.count(event->cookie)) {
                it->new_path = move_file_storage[event->cookie];
                fileCache_->moveDir(entry_path, it->new_path);
                watch_map[it->wd] = it->new_path;        // update the map
                move_file_storage.erase(event->cookie);  // erase the new_path storage
                move_dir_storage.erase(it);              // erase the move_dir_storage entry
              } else {
                it->cookie = event->cookie;
              }
              goto end;
            }
          }
          // did not find
          MoveDirStorage tmp;
          tmp.cookie = event->cookie;
          tmp.old_path = entry_path;
          if (move_file_storage.count(event->cookie)) {
            tmp.new_path = move_file_storage[event->cookie];
          }
          move_dir_storage.push_back(tmp);
        } else {
          std::cout << "File moved from, cookie: " << event->cookie << std::endl;
          // store the file's move info or do user defined action
          if (move_file_storage.count(event->cookie)) {
            fileCache_->moveFile(entry_path, move_file_storage[event->cookie]);
            move_file_storage.erase(event->cookie);  // erase the new_path storage;
          } else {
            move_file_storage[event->cookie] = entry_path;
          }
        }
      } else if (event->mask &
                 IN_MOVED_TO) {  // IN_MOVED_TO is strictly after IN_MOVED_FROM in the read buffer of inotify
        if (event->mask & IN_ISDIR) {
          std::cout << "Dir moved to, cookie: " << event->cookie << std::endl;
          // search vector<MoveDirStorage> by cookie to find the corresponding IN_MOVED_FROM
          for (auto it = move_dir_storage.begin(); it != move_dir_storage.end(); ++it) {
            if (it->cookie == event->cookie) {
              it->new_path = entry_path;
              if (it->wd > 0) {
                // we have collected all the info we need, no need to insert to the move_file_storage
                watch_map[it->wd] = entry_path;  // update the map
                fileCache_->moveDir(it->old_path, it->new_path);
                move_dir_storage.erase(it);  // erase the move_dir_storage entry
                goto end;
              }
            }
          }
          move_file_storage[event->cookie] = entry_path;
        } else {
          std::cout << "File moved to, cookie: " << event->cookie << std::endl;
          if (move_file_storage.count(event->cookie)) {
            fileCache_->moveFile(move_file_storage[event->cookie], entry_path);
            move_file_storage.erase(event->cookie);  // erase the new_path storage;
          } else {
            move_file_storage[event->cookie] = entry_path;
          }
        }
      } else if (event->mask & IN_MOVE_SELF) {
        /* we only watch dir here, so no need to handle file's move */
        std::cout << "Dir moved self" << std::endl;
        // search vector<MoveDirStorage> by old_path to find the corresponding IN_MOVED_FROM
        for (auto it = move_dir_storage.begin(); it != move_dir_storage.end(); ++it) {
          if (it->old_path == parent_path) {
            if (move_file_storage.count(it->cookie)) {
              move_file_storage.erase(it->cookie);  // erase the new_path storage;
              watch_map[event->wd] = it->new_path;  // update the map, new_path in move_dir_storage already set, no need
                                                    // to retrieve it from move_file_storage
              fileCache_->moveDir(it->old_path, it->new_path);
              move_dir_storage.erase(it);  // erase the move_dir_storage entry
            } else {
              it->wd = event->wd;
            }
            goto end;
          }
        }
        MoveDirStorage tmp;
        tmp.wd = event->wd;
        tmp.old_path = parent_path;
        move_dir_storage.push_back(tmp);
      } else if (event->mask & IN_IGNORED) {
        std::cout << "Dir ignored" << std::endl;
        watch_map.erase(event->wd);  // remove the watch
      }
    end:
      i += sizeof(struct inotify_event) + event->len;
    }
  });
  inotifyChannel_->flushEvent();
}
