#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/CurrentThread.h"
#include "base/FileUtil.h"
#include "base/Exception.h"
#include "http/HTTP-Request.h"
#include "http/HTTP-Response.h"
#include "http/HTTP-Server.h"
#include "log/AsyncLogging.h"
#include "tcp/EventLoop.h"
#include "tcp/Poller.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"
#include "tcp/Thread.h"
#include "tcp/ThreadPool.h"

namespace fs = std::filesystem;
const fs::path g_staticPath = "/home/wzy/code/cpp-server/static/";

int main() {
  // std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::init("log/single-client");
  // async_logging->start();

  std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
  HTTPServer* server = new HTTPServer(loop.get(), "0.0.0.0", 5000, g_staticPath);

  server->onResponse([server](const HTTPRequest* req, HTTPResponse* res) {
    using Status = HTTPResponse::Status;
    using ContentType = HTTPResponse::ContentType;
    std::string url = req->url();
    LOG_TRACE << "HTTP request: " << '"' << url << '"';
    if (req->method() == HTTPRequest::Method::GET) {
      if (url == "/") {
        res->setStatus(Status::OK);
        res->setContentType(ContentType::text_html);
        res->setBody(server->getFile("/linux_cmd.html"));
      } else if (url.find("/download") == 0) {
        // TODO check the file size, if it's too big, use sendFile instead
        std::shared_ptr<Buffer> buf;
        try {
          buf = server->getFile(url);
          if (buf == nullptr) {
            LOG_ERROR << "HTTPServer::onResponse, file not found";
            res->setClose(true);
            res->setStatus(Status::NotFound);
            return;
          }
          res->setBody(buf);
          res->setStatus(Status::OK);
          res->setContentType(ContentType::application_octet_stream);
          res->addHeader("Content-Disposition", "attachment; filename=" + fs::path(url).filename().string());
        } catch (const bigfile_error& e) {
          fs::path file_path = g_staticPath / fs::path(url).relative_path();
          int file_fd = ::open(file_path.c_str(), O_RDONLY);
          if (file_fd == -1) {
            LOG_ERROR << "TCPConnection::sendFile, open file failed: " << file_path;
            res->setClose(true);
            res->setStatus(Status::NotFound);
            return;
          }
          struct stat st;
          ::fstat(file_fd, &st);

          res->setBigFile(file_fd);
          res->setStatus(Status::OK);
          res->setContentType(ContentType::application_octet_stream);
          res->addHeader("Content-Length", std::to_string(st.st_size));
          res->addHeader("Content-Disposition", "attachment; filename=" + fs::path(url).filename().string());
        }
      } else {
        std::shared_ptr<Buffer> buf;
        try {
           buf = server->getFile(url);
           if (buf == nullptr) {
             LOG_ERROR << "HTTPServer::onResponse, file not found";
             res->setClose(true);
             res->setStatus(Status::NotFound);
             return;
           }
           res->setBody(buf);
           res->setStatus(Status::OK);
           res->setContentType(fs::path(url).extension().string());
        } catch (const bigfile_error& e) {
          fs::path file_path = g_staticPath / fs::path(url).relative_path();
          int file_fd = ::open(file_path.c_str(), O_RDONLY);
          if (file_fd == -1) {
            LOG_ERROR << "TCPConnection::sendFile, open file failed: " << file_path;
            res->setClose(true);
            res->setStatus(Status::NotFound);
            return;
          }
          struct stat st;
          ::fstat(file_fd, &st);

          res->setBigFile(file_fd);
          res->setStatus(Status::OK);
          res->setContentType(ContentType::application_octet_stream);
          res->addHeader("Content-Length", std::to_string(st.st_size));
          res->addHeader("Content-Disposition", "attachment; filename=" + fs::path(url).filename().string());
        }
      }
    } else {
      res->setStatus(Status::NotImplemented);
      res->setClose(true);
    }
  });

  server->start();

  return 0;
}
