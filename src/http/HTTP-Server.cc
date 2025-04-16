#include "HTTP-Server.h"

#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <utility>

#include "HTTP-Connection.h"
#include "HTTP-Context.h"
#include "HTTP-Request.h"
#include "HTTP-Response.h"
#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "base/Util.h"
#include "log/Logger.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"

#define CONNECTION_TIMEOUT_SECOND 900.

HTTPServer::HTTPServer(EventLoop *loop, const char *ip, const int port) : loop_(loop) {
  tcp_server_ = std::make_unique<TCPServer>(loop, ip, port);

  /**
   * set TCPServer::on_connection_callback_
   */
  tcp_server_->OnConnection([this](const std::shared_ptr<TCPConnection> &conn) {
    int fd_clnt = conn->GetFD();

    // init the HTTPContext
    conn->SetContext(new HTTPContext());

    // set timer for timeout close
    conn->SetTimer(loop_->RunEvery(CONNECTION_TIMEOUT_SECOND, [this, conn]() {
      if (conn->GetConnectionState() == TCPState::Connected) {
        if (conn->GetLastActiveTime() + CONNECTION_TIMEOUT_SECOND < TimeStamp::Now()) {
          // cacel the timer first
          loop_->CanelTimer(conn->GetTimer());
          conn->HandleClose();
        }
      } else if (conn->GetConnectionState() == TCPState::Disconnected) {
        // if the connection is already closed, cancel the timer
        loop_->CanelTimer(conn->GetTimer());
      } else {
        LOG_WARN << "HTTPServer::HTTPServer, unknown state";
      }
    }));

    // print out peer's info
    struct sockaddr_in addr_peer;
    socklen_t addrlength_peer = sizeof(addr_peer);
    getpeername(fd_clnt, (struct sockaddr *)&addr_peer, &addrlength_peer);
    struct sockaddr_in addr_local;
    socklen_t addrlength_local = sizeof(addr_local);
    getsockname(fd_clnt, (struct sockaddr *)&addr_local, &addrlength_local);
    std::cout << "tid-" << CurrentThread::gettid() << " Connection"
              << "[fd#" << fd_clnt << "]"
              << " from " << inet_ntoa(addr_peer.sin_addr) << ":" << ntohs(addr_peer.sin_port) << " to "
              << inet_ntoa(addr_local.sin_addr) << ":" << ntohs(addr_local.sin_port) << std::endl;
  });

  /**
   * set TCPConnection::on_message_callback_
   */
  tcp_server_->OnMessage([this](const std::shared_ptr<TCPConnection> &conn) {
    if (TCPState::Connected != conn->GetConnectionState()) {
      LOG_WARN << "HTTPConnection::EnableHTTPConnection, on_message_callback_ called while disconnected";
      return;
    }
    using HTTPRequestParseState = HTTPContext::HTTPRequestParseState;
    HTTPRequestParseState return_state;
    HTTPContext *http_context_ = static_cast<HTTPContext *>(conn->GetContext());
    return_state = http_context_->ParseRequest(conn->readBuffer()->peek(), conn->readBuffer()->readableBytes());
    conn->readBuffer()->retrieveAll();
    switch (return_state) {
      case HTTPRequestParseState::COMPLETE: {
        HTTPRequest *req = http_context_->GetHTTPRequest();
        // check if client wish to keep the connection
        std::string conn_state = req->GetHeaderByKey("Connection");
        bool is_clnt_want_to_close = (util::toLower(conn_state) == "close");
        // generate response
        HTTPResponse res(is_clnt_want_to_close);
        // set the response according to the request
        on_response_callback_(http_context_->GetHTTPRequest(), &res);
        // send message
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        // reset the state of the parser and the snapshot_
        http_context_->ResetState();
        // if the client wish to close connection
        if (res.IsClose()) {
          if (conn->GetConnectionState() == TCPState::Disconnected) {
            break;
          }
          conn->HandleClose();
        }
        break;
      }
      // handle invalid cases
      case HTTPRequestParseState::INVALID_METHOD: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid HTTP METHOD";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::NotImplemented);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_URL: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid URL";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::NotFound);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_PROTOCOL: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid PROTOCOL";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::HTTPVersionNotSupported);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_HEADER: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid HEADER";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::Forbidden);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_CRLF: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid CRLF";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::BadRequest);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      // this one will close the connection
      case HTTPRequestParseState::INVALID: {
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, just invalid";
        HTTPResponse res(true, HTTPResponse::HTTPStatus::BadRequest);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        if (conn->GetConnectionState() == TCPState::Disconnected) {
          break;
        }
        conn->HandleClose();
        break;
      }
      // UNFINISHED, waiting for more data
      default: {
        break;
      }
    }
  });
}

HTTPServer::~HTTPServer() {}

void HTTPServer::OnResponse(std::function<void(const HTTPRequest *, HTTPResponse *)> cb) {
  on_response_callback_ = std::move(cb);
}

void HTTPServer::Start() { tcp_server_->Start(); }
