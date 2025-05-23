#include "Thread.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "EventLoop.h"

Thread::Thread() {}

Thread::~Thread() {}

std::shared_ptr<EventLoop> Thread::startLoop() {
  // create sub thread, sub thread will: create EventLoop and call EventLoop->loop()
  thread_ = std::thread([this]() {
    {
      std::unique_lock<std::mutex> lock(mtx_);
      loop_ = std::make_shared<EventLoop>();
      cv_.notify_one();
    }
    loop_->loop();
  });
  // wait until the sub thread has create the EventLoop, then return the EventLoop to main thread
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return loop_ != nullptr; });
    return loop_;
  }
}
