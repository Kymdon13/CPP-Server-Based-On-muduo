#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "base/cppserver-common.h"

class Poller;
class Timer;
class TimeStamp;
class TimerQueue;

class EventLoop {
 private:
  const pid_t tid_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timer_queue_;
  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;
  // no need to init in the init list
  std::mutex pendingFunctorsMutex_;
  std::vector<std::function<void()>> pendingFunctors_;
  bool quit_ = false;

  void doPendingFunctors();
  void wake();

 public:
  DISABLE_COPYING_AND_MOVING(EventLoop);
  EventLoop();
  ~EventLoop();

  // for other threads to call, cause the thread that created this EventLoop is looping
  void Quit();

  void Loop();
  void UpdateChannel(Channel *channel) const;
  void DeleteChannel(Channel *channel) const;

  /// @brief check if it is the thread that created this EventLoop
  bool IsLocalThread();

  void CallOrQueue(std::function<void()> cb);

  std::shared_ptr<Timer> RunAt(TimeStamp time, std::function<void()> cb);
  std::shared_ptr<Timer> RunAfter(double delay, std::function<void()> cb);
  std::shared_ptr<Timer> RunEvery(double interval, std::function<void()> cb);
  void canelTimer(const std::shared_ptr<Timer> &timer);
};
