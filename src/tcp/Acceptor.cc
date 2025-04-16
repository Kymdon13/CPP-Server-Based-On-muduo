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
#include "log/Logger.h"
#include "tcp/TCP-Connection.h"

Acceptor::Acceptor(EventLoop *loop, const char *ip, int port) : loop_(loop), sockfd_(-1) {
  create();
  bind(ip, port);
  listen();
  channel_ = std::make_unique<Channel>(sockfd_, loop, true, false, false, false);  // use Level Trigger

  // register accept connection callback in the acceptor's channel
  channel_->setReadCallback([this]() {
    // check if the sockfd_ is invalid
    if (-1 == sockfd_) {
      LOG_SYSERR << "Acceptor::Acceptor, sockfd_ invalid";
      return;
    }
    // init the addr struct
    struct sockaddr_in addr_client;
    socklen_t addr_client_len = sizeof(addr_client);
    // accept the connection
    int fd_client =
        ::accept4(sockfd_, (struct sockaddr *)&addr_client, &addr_client_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (-1 == fd_client) LOG_SYSERR << "Acceptor::Acceptor, accept4 failed";
    // call on_new_connection_callback_
    on_new_connection_callback_(fd_client);
  });
  channel_->flushEvent();
}

Acceptor::~Acceptor() {
  loop_->deleteChannel(channel_.get());
  ::close(sockfd_);
}

void Acceptor::create() {
  if (-1 != sockfd_) {
    LOG_WARN << "Acceptor::create, Acceptor already created";
    return;
  }
  sockfd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);  // listening socket should be blocking I/O

  if (-1 == sockfd_) {
    LOG_SYSERR << "Acceptor::create, socket failed";
    return;
  }

  // set the SO_REUSEADDR
  int optval = 1;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Acceptor::bind(const char *ip, int port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);
  if (-1 == ::bind(sockfd_, (struct sockaddr *)&addr, sizeof(addr))) {
    LOG_SYSERR << "Acceptor::bind, bind failed";
    return;
  }
}

void Acceptor::listen() {
  if (-1 == sockfd_) {
    LOG_ERROR << "Acceptor::listen, sockfd_ invalid";
    return;
  }
  if (-1 == ::listen(sockfd_, SOMAXCONN)) {
    LOG_SYSERR << "Acceptor::listen, socket listened failed";
  }
}

void Acceptor::onNewConnection(std::function<void(int)> cb) { on_new_connection_callback_ = std::move(cb); }
