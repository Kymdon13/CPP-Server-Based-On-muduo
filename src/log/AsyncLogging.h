#pragma once

#include <sys/time.h>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

#include "LogStream.h"
#include "Logger.h"
#include "base/Latch.h"
#include "base/cppserver-common.h"

class AsyncLogging {
 private:
  using Buffer = FixedBuffer<FIXED_BUFFER_SIZE_BIG>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = std::unique_ptr<Buffer>;

  void threadFunc();

  bool running_;
  const std::string dir_;
  const size_t rollSize_;
  const time_t flushInterval_;

  // for thread safe
  Latch latch_;
  std::mutex mutex_;
  std::condition_variable cond_;
  // buffer
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;

  std::thread thread_;

 public:
  DISABLE_COPYING_AND_MOVING(AsyncLogging);

  AsyncLogging(const std::string &dir, size_t rollSize, time_t flushInterval);
  ~AsyncLogging();

  void append(const char *logline, int len);
  void start() {
    running_ = true;
    thread_ = std::thread(&AsyncLogging::threadFunc, this);
    // wait for the thread to be ready
    latch_.wait();
    std::cout << "[logging thread] - AsyncLogging ready to go..." << std::endl;
  }
  void stop() {
    running_ = false;
    cond_.notify_all();
    thread_.join();
  }

  // init the g_output & g_flushBeforeAbort
  static std::shared_ptr<AsyncLogging> Init(const std::string &dir = "log", size_t rollSize = 100 * 1024,
                                            time_t flushInterval = 3) {
    std::shared_ptr<AsyncLogging> asyncLogger = std::make_unique<AsyncLogging>(dir, rollSize, flushInterval);
    Logger::setOutput([asyncLogger](const char *msg, int len) { asyncLogger->append(msg, len); });
    Logger::setFlush([asyncLogger]() {
      if (asyncLogger->running_) {
        asyncLogger->stop();
      }
    });
    return asyncLogger;
  }
};
