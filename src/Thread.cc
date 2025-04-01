#include "include/Thread.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "include/EventLoop.h"

Thread::Thread() {}

Thread::~Thread() {}

std::shared_ptr<EventLoop> Thread::StartLoop() {
  // create sub thread, sub thread will: create EventLoop and call EventLoop->Loop()
  thread_ = std::thread([this]() {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      loop_ = std::make_shared<EventLoop>();
      cv_.notify_one();
    }
    loop_->Loop();
  });
  // wait until the sub thread has create the EventLoop, then return the EventLoop to main thread
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return loop_ != nullptr; });
    return loop_;
  }
}
