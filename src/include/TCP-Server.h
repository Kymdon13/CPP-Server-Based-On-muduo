#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Acceptor.h"
#include "ThreadPool.h"
#include "cppserver-common.h"

class TCPServer {
 private:
  std::unique_ptr<EventLoop> main_reactor_;
  std::vector<std::unique_ptr<EventLoop>> sub_reactors_;

  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, std::shared_ptr<TCPConnection>> connection_map_;

  std::unique_ptr<ThreadPool> thread_pool_;

  /// @brief for developer to customize
  std::function<void(std::shared_ptr<TCPConnection>)> on_connection_callback_;
  /// @brief for developer to customize
  std::function<void(std::shared_ptr<TCPConnection>)> on_message_callback_;

  size_t next_connection_id_{0};

 public:
  DISABLE_COPYING_AND_MOVING(TCPServer);
  TCPServer(const char *ip, const int port);
  ~TCPServer() = default;

  void Start();

  /// @brief for developer to customize (for example, you want to print something when connected), the
  /// Acceptor->on_new_connection_callback_ is registered by the ctor
  void OnConnection(std::function<void(std::shared_ptr<TCPConnection>)> const &func);
  /// @brief for developer to customize
  void OnMessage(std::function<void(std::shared_ptr<TCPConnection>)> const &func);
};
