#include "Acceptor.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "Channel.h"
#include "EventLoop.h"
#include "Exception.h"

Acceptor::Acceptor(EventLoop *loop, const char *ip, int port) : loop_(loop), listen_fd_(-1) {
  Create();
  Bind(ip, port);
  Listen();
  channel_ = std::make_unique<Channel>(listen_fd_, loop, true, false, false);  // use Level Trigger
  std::function<void()> cb = std::bind(&Acceptor::AcceptConnectionCallback, this);
  channel_->SetReadCallback(cb);
}

Acceptor::~Acceptor() {
  loop_->DeleteChannel(channel_.get());
  ::close(listen_fd_);
};

void Acceptor::Create() {
  if (-1 != listen_fd_) {
    WarnIf(true, "Acceptor already created");
    return;
  }
  listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);  // listening socket should be blocking I/O
  WarnIf(-1 == listen_fd_, "Acceptor::Create failed");
}

void Acceptor::Bind(const char *ip, int port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);
  WarnIf(-1 == ::bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr)), "Acceptor::Bind failed");
}

void Acceptor::Listen() {
  if (-1 == listen_fd_) {
    WarnIf(true, "Acceptor::listen_fd_ not valid");
    return;
  }
  WarnIf(-1 == ::listen(listen_fd_, SOMAXCONN), "Acceptor::Listen failed");
}

void Acceptor::AcceptConnectionCallback() {
  struct sockaddr_in addr_client;
  socklen_t addr_client_len = sizeof(addr_client);
  if (-1 == listen_fd_) {
    WarnIf(true, "Acceptor::listen_fd_ not valid");
    return;
  }

  int fd_client =
      ::accept4(listen_fd_, (struct sockaddr *)&addr_client, &addr_client_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  WarnIf(-1 == fd_client, "accept4 failed");

  if (!new_connection_callback_) {
    WarnIf(true, "Acceptor::AcceptConnectionCallback failed: new_connection_callback_ is none");
    return;
  } else {
    new_connection_callback_(fd_client);
  }
}
