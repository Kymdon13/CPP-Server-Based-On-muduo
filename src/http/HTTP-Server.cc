#include "HTTP-Server.h"

#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <utility>

#include "HTTP-Connection.h"
#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "tcp/Buffer.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"

#define CONNECTION_TIMEOUT 30.

HTTPServer::HTTPServer(EventLoop *loop, const char *ip, const int port) : loop_(loop) {
  tcp_server_ = std::make_unique<TCPServer>(loop, ip, port);
  /**
   * set TCPServer::on_connection_callback_
   */
  tcp_server_->OnConnection([this](const std::shared_ptr<TCPConnection> &conn) {
    int fd_clnt = conn->GetFD();

    // set timer for timeout close
    conn->SetTimer(loop_->RunEvery(CONNECTION_TIMEOUT, [this, conn]() {
      if (conn->GetConnectionState() == TCPState::Connected) {
        if (conn->GetLastActiveTime() + CONNECTION_TIMEOUT < TimeStamp::Now()) {
          // cacel the timer first
          loop_->CanelTimer(conn->GetTimer());
          conn->HandleClose();
        }
      } else if (conn->GetConnectionState() == TCPState::Disconnected) {
        // if the connection is already closed, cancel the timer
        loop_->CanelTimer(conn->GetTimer());
      } else {
        WarnIf(true, "TCPServer::on_connection_callback_(): unknown state");
      }
    }));

    // create HTTPConnection, and set its on_message_callback_
    http_connection_map_[fd_clnt] = std::make_unique<HTTPConnection>(conn);
    http_connection_map_[fd_clnt]->OnMessage(on_message_callback_);

    // set TCPConnection::on_message_callback_ through HTTPConnection, provides more customizability (you can customize
    // the EnableHTTPConnection function, for example, set different message callback according to different ip)
    http_connection_map_[fd_clnt]->EnableHTTPConnection();

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
   * set TCPServer::on_close_callback_, erase HTTPConnection from http_connection_map_
   */
  tcp_server_->OnClose([this](const std::shared_ptr<TCPConnection> &conn) {
    int fd_clnt = conn->GetFD();
    auto it = http_connection_map_.find(fd_clnt);
    if (it == http_connection_map_.end()) {
      std::stringstream ss;
      ss << "HTTPServer->tcp_server->OnClose(): can not find fd: " << fd_clnt << " in http_connection_map_";
      WarnIf(true, ss.str().c_str());
      return;
    }
    http_connection_map_.erase(fd_clnt);
  });
}

HTTPServer::~HTTPServer() {}

void HTTPServer::OnMessage(std::function<void(const HTTPRequest *, HTTPResponse *)> cb) {
  on_message_callback_ = std::move(cb);
}

void HTTPServer::Start() { tcp_server_->Start(); }
