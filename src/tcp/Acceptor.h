#pragma once

#include <functional>
#include <memory>

#include "base/common.h"

class TCPConnection;
class Channel;
class EventLoop;

class Acceptor {
 private:
  EventLoop* loop_;
  int sockfd_;
  std::unique_ptr<Channel> channel_;
  std::function<void(int)> on_new_connection_callback_;

 public:
  DISABLE_COPYING_AND_MOVING(Acceptor);
  Acceptor(EventLoop* loop, const char* ip, int port);
  ~Acceptor();

  void create();

  void bind(const char* ip, int port);

  void listen();

  void onNewConnection(std::function<void(int)> cb);
};
