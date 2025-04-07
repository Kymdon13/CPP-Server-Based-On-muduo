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

class HTTPServer {
 private:
  std::unique_ptr<TCPServer> tcp_server_;
  std::function<void(const HTTPRequest *, HTTPResponse *)> response_callback_;

  std::unordered_map<int, std::unique_ptr<HTTPConnection>> http_connection_map_;

 public:
  DISABLE_COPYING_AND_MOVING(HTTPServer);
  HTTPServer(const char *ip, const int port);
  ~HTTPServer();

  void SetResponseCallback(std::function<void(const HTTPRequest *, HTTPResponse *)> cb);

  void OnMessage(const std::shared_ptr<TCPConnection> &conn);

  void Start();
};
