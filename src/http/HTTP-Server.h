#pragma once

#include <stdio.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "HTTP-Context.h"
#include "base/common.h"

class TCPServer;
class TCPConnection;
class HTTPRequest;
class HTTPResponse;
class HTTPConnection;
class EventLoop;

class HTTPServer {
 private:
  EventLoop *loop_;
  std::unique_ptr<TCPServer> tcpServer_;
  std::function<void(const HTTPRequest *, HTTPResponse *)> on_response_callback_;

 public:
  DISABLE_COPYING_AND_MOVING(HTTPServer);
  HTTPServer(EventLoop *loop, const char *ip, const int port);
  ~HTTPServer();

  void onResponse(std::function<void(const HTTPRequest *, HTTPResponse *)> cb);

  void start();
};
