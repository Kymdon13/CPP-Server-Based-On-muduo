#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "base/common.h"

class Poller;
class Timer;
class TimeStamp;
class TimerQueue;
class Channel;

class EventLoop {
 private:
  const pid_t tid_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timerQueue_;
  int wakefd_;
  std::unique_ptr<Channel> wakeChannel_;
  // no need to init in the init list
  std::mutex pendingFunctorsMutex_;
  std::vector<std::function<void()>> pendingFunctors_;
  bool quit_ = false;
  // workload of EventLoop
  typedef struct Workload {
    std::atomic<size_t> connections_{0};
    std::atomic<off_t> left_bytes_{0};
  } Workload;
  Workload workload_;

  void doPendingFunctors();
  void wake();

 public:
  DISABLE_COPYING_AND_MOVING(EventLoop);
  EventLoop();
  ~EventLoop();

  // for other threads to call, cause the thread that created this EventLoop is looping
  void quit();

  void loop();
  void updateChannel(Channel* channel) const;
  void deleteChannel(Channel* channel) const;

  /// @brief check if it is the thread that created this EventLoop
  bool isLocalThread();

  void callOrQueue(std::function<void()> cb);
  void queueFunctor(std::function<void()> cb);

  std::shared_ptr<Timer> runAt(TimeStamp time, std::function<void()> cb);
  std::shared_ptr<Timer> runAfter(double delay, std::function<void()> cb);
  std::shared_ptr<Timer> runEvery(double interval, std::function<void()> cb);
  void canelTimer(const std::shared_ptr<Timer>& timer);

  // workload related
  Workload* workload() { return &workload_; }
  // calculate the workload (1MB equals 1 connection)
  size_t load() { return workload_.connections_ + static_cast<size_t>(workload_.left_bytes_ >> 20); }
};
