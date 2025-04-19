#include "HTTP-Server.h"

#include <arpa/inet.h>

#include <iostream>
#include <sstream>
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

HTTPServer::HTTPServer(EventLoop* loop, const char* ip, const int port) : loop_(loop) {
  tcpServer_ = std::make_unique<TCPServer>(loop, ip, port);

  /**
   * set TCPServer::on_connection_callback_
   */
  tcpServer_->onConnection([this](const std::shared_ptr<TCPConnection>& conn) {
    int fd_clnt = conn->fd();

    // init the HTTPContext
    conn->setContext(new HTTPContext());

    // set timer for timeout close
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
    ParseState return_state;  // FIXME
    HTTPContext* http_context_ = static_cast<HTTPContext*>(conn->context());
    return_state = http_context_->parseRequest(conn->inBuffer());
    switch (return_state) {
      case ParseState::COMPLETE: {
        HTTPRequest* req = http_context_->getRequest();
        // check if client wish to keep the connection
        std::string conn_state = req->getHeaderByKey("Connection");
        bool is_clnt_want_to_close = (util::toLower(conn_state) == "close");
        // generate response
        HTTPResponse res(is_clnt_want_to_close);
        // set the response according to the request
        on_response_callback_(http_context_->getRequest(), &res);
        // send message
        conn->send(res.getResponse()->peek(), res.getResponse()->readableBytes());
        // reset the state of the parser and the snapshot_
        http_context_->reset();
        // if the client wish to close connection
        if (res.isClose()) {
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
}
