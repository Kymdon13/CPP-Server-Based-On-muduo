#pragma once

#include <functional>
#include <memory>

#include "cppserver-common.h"

class Acceptor {
 private:
  EventLoop *loop_;
  int listen_fd_;
  std::unique_ptr<Channel> channel_;
  std::function<void(int)> on_new_connection_callback_;

 public:
  DISABLE_COPYING_AND_MOVING(Acceptor);
  Acceptor(EventLoop *loop, const char *ip, int port);
  ~Acceptor();

  void Create();

  void Bind(const char *ip, int port);

  void Listen();

  void OnNewConnection(std::function<void(int)> const &cb);
};
