#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "EventLoop.h"
#include "base/common.h"

class Thread;

class ThreadPool {
 private:
  std::unique_ptr<EventLoop> mainReactor_;

  std::vector<std::unique_ptr<Thread>> threads_;
  size_t n_threads_;

  std::vector<std::shared_ptr<EventLoop>> loops_;

 public:
  DISABLE_COPYING_AND_MOVING(ThreadPool);
  /// @brief create a ThreadPool
  /// @param n_threads how many threads you want to create in the ThreadPool
  explicit ThreadPool(EventLoop* loop, int n_threads);
  ~ThreadPool();

  void init();

  /// @brief the main caller TCPConnection::TCPConnection() will not participate in the life management of EventLoop, so
  /// we return EventLoop* instead of shared_ptr<EventLoop>
  EventLoop* getSubReactor();
};
