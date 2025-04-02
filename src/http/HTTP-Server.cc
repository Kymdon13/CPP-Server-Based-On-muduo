#include "HTTP-Server.h"

#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <utility>

#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "HTTP-Connection.h"
#include "tcp/Buffer.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"

HTTPServer::HTTPServer(const char *ip, const int port) {
  tcp_server_ = std::make_unique<TCPServer>(ip, port);

  /**
   * set TCPServer::on_connection_callback_
   */
  tcp_server_->OnConnection([this](const std::shared_ptr<TCPConnection> &conn) {
    int fd_clnt = conn->GetFD();

    // create HTTPConnection, and set its response_callback_
    http_connection_map_[fd_clnt] = std::make_unique<HTTPConnection>(conn);
    http_connection_map_[fd_clnt]->SetResponseCallback(response_callback_);

    // set TCPConnection::on_message_callback_ through HTTPConnection, provides more customizability (you can custom the
    // SetTCPConnectionOnMessageCallback function, for example, set different message callback according to different
    // ip)
    // BUG(wzy)
    http_connection_map_[fd_clnt]->SetTCPConnectionOnMessageCallback();
    // conn->OnMessage([](std::shared_ptr<TCPConnection> conn){
    //     std::cout << "Message from client [fd#" << conn->GetFD() << "]: " << conn->GetReadBuffer()->GetBuffer() << std::endl;
    //     conn->Send(conn->GetReadBuffer()->GetBuffer());
    // });

    // print out peer's info
    struct sockaddr_in addr_peer;
    socklen_t addrlength_peer = sizeof(addr_peer);
    getpeername(fd_clnt, (struct sockaddr *)&addr_peer, &addrlength_peer);
    std::cout << "tid-" << CurrentThread::gettid() << " Connection"
              << "[fd#" << fd_clnt << "]"
              << " from " << inet_ntoa(addr_peer.sin_addr) << ":" << ntohs(addr_peer.sin_port) << std::endl;
  });

  /**
   * set TCPServer::on_close_callback_, erase HTTPConnection from http_connection_map_
   */
  tcp_server_->OnClose([this](TCPConnection *conn) {
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

void HTTPServer::SetResponseCallback(const std::function<void(const HTTPRequest *, HTTPResponse *)> &cb) {
  response_callback_ = std::move(cb);
}

void HTTPServer::Start() { tcp_server_->Start(); }
