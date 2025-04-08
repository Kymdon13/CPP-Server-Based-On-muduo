#pragma once

#include <stdio.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "base/cppserver-common.h"

class TCPServer;
class TCPConnection;
class HTTPRequest;
class HTTPResponse;
class HTTPConnection;
class EventLoop;

class HTTPServer {
 private:
  EventLoop *loop_;
  std::unique_ptr<TCPServer> tcp_server_;
  std::function<void(const HTTPRequest *, HTTPResponse *)> on_message_callback_;

  std::unordered_map<int, std::unique_ptr<HTTPConnection>> http_connection_map_;

 public:
  DISABLE_COPYING_AND_MOVING(HTTPServer);
  HTTPServer(EventLoop* loop, const char *ip, const int port);
  ~HTTPServer();

  void OnMessage(std::function<void(const HTTPRequest *, HTTPResponse *)> cb);

  void Start();
};
