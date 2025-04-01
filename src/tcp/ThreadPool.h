#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "EventLoop.h"
#include "cppserver-common.h"

class Thread;

class ThreadPool {
 private:
  std::unique_ptr<EventLoop> main_reactor_;

  std::vector<std::unique_ptr<Thread>> threads_;
  size_t n_threads_;
  size_t which_sub_reactor_{0};

  std::vector<std::shared_ptr<EventLoop>> loops_;

 public:
  DISABLE_COPYING_AND_MOVING(ThreadPool);
  /// @brief create a ThreadPool
  /// @param n_threads how many threads you want to create in the ThreadPool
  explicit ThreadPool(int n_threads);
  ~ThreadPool();

  void Init();

  /// @brief the main caller TCPConnection::TCPConnection() will not participate in the life management of EventLoop, so
  /// we return EventLoop* instead of shared_ptr<EventLoop>
  EventLoop *GetSubReactor();
};
