#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "cppserver-common.h"

class TCPServer {
 private:
  std::unique_ptr<EventLoop> main_reactor_;
  std::vector<std::unique_ptr<EventLoop>> sub_reactors_;

  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, TCPConnection *> connection_map_;

  std::unique_ptr<ThreadPool> thread_pool_;

  std::function<void(TCPConnection *)> on_connect_callback_;
  std::function<void(TCPConnection *)> on_message_callback_;

  size_t next_connection_id_{0};

 public:
  DISABLE_COPYING_AND_MOVING(TCPServer);
  TCPServer(const char *ip, const int port);
  ~TCPServer() = default;

  void Start();

  void OnConnect(std::function<void(TCPConnection *)> const &func);
  void OnMessage(std::function<void(TCPConnection *)> const &func);

  void HandleClose(int fd);
  void HandleNewConnection(int fd);
};
