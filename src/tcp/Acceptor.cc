#include "Acceptor.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <utility>

#include "Channel.h"
#include "EventLoop.h"
#include "base/Exception.h"
#include "tcp/TCP-Connection.h"

Acceptor::Acceptor(EventLoop *loop, const char *ip, int port) : loop_(loop), listen_fd_(-1) {
  Create();
  Bind(ip, port);
  Listen();
  channel_ = std::make_unique<Channel>(listen_fd_, loop, true, false, false, false);  // use Level Trigger

  // register accept connection callback in the acceptor's channel
  channel_->SetReadCallback([this]() {
    // check if the listen_fd_ is invalid
    if (-1 == listen_fd_) {
      WarnIf(true, "Acceptor::listen_fd_ not valid");
      return;
    }
    // init the addr struct
    struct sockaddr_in addr_client;
    socklen_t addr_client_len = sizeof(addr_client);
    // accept the connection
    int fd_client =
        ::accept4(listen_fd_, (struct sockaddr *)&addr_client, &addr_client_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    WarnIf(-1 == fd_client, "accept4 failed");
    // call on_new_connection_callback_
    on_new_connection_callback_(fd_client);
  });
  channel_->FlushEvent();
}

Acceptor::~Acceptor() {
  loop_->DeleteChannel(channel_.get());
  ::close(listen_fd_);
}

void Acceptor::Create() {
  if (-1 != listen_fd_) {
    WarnIf(true, "Acceptor already created");
    return;
  }
  listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);  // listening socket should be blocking I/O

  WarnIf(-1 == listen_fd_, "Acceptor::Create failed");

  // set the SO_REUSEADDR
  int optval = 1;
  ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Acceptor::Bind(const char *ip, int port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);
  ErrorIf(-1 == ::bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr)), "Acceptor::Bind failed");
}

void Acceptor::Listen() {
  if (-1 == listen_fd_) {
    WarnIf(true, "Acceptor::listen_fd_ not valid");
    return;
  }
  WarnIf(-1 == ::listen(listen_fd_, SOMAXCONN), "Acceptor::Listen failed");
}

void Acceptor::OnNewConnection(std::function<void(int)> cb) { on_new_connection_callback_ = std::move(cb); }
