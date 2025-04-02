#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "base/cppserver-common.h"

class EventLoop;

class Thread {
 private:
  std::shared_ptr<EventLoop> loop_{nullptr};
  std::thread thread_;
  std::mutex mtx_;
  std::condition_variable cv_;

 public:
  DISABLE_COPYING_AND_MOVING(Thread);
  Thread();
  ~Thread();

  std::shared_ptr<EventLoop> StartLoop();
};
