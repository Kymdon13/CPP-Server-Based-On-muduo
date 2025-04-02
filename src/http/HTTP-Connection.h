#pragma once

#include <functional>
#include <memory>

#include "base/cppserver-common.h"

class TCPConnection;
class HTTPContext;
class HTTPRequest;
class HTTPResponse;

class HTTPConnection {
 private:
  std::shared_ptr<TCPConnection> tcp_connection_;
  std::unique_ptr<HTTPContext> http_context_;
  std::function<void(const HTTPRequest *, HTTPResponse *)> response_callback_;

 public:
  DISABLE_COPYING_AND_MOVING(HTTPConnection);
  HTTPConnection(const std::shared_ptr<TCPConnection> &conn);
  ~HTTPConnection();

  void SetResponseCallback(std::function<void(const HTTPRequest *, HTTPResponse *)> cb);

  // directly set the on_message_callback_ of TCPConnection, so we can have more customizability,
  // and that TCPConnection::EnableConnection executes after Acceptor::OnNewConnection,
  // it makes sure that Acceptor will not override the on_message_callback_ we set.
  void SetTCPConnectionOnMessageCallback();
};