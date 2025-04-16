#pragma once

#include <sys/timerfd.h>

#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "TimeStamp.h"
#include "tcp/Channel.h"

class Timer;
class EventLoop;
class Channel;

class TimerQueue {
 private:
  using Entry = std::pair<TimeStamp, std::shared_ptr<Timer>>;
  // make sure the nullptr is the biggest
  struct Compare {
    bool operator()(const Entry &lhs, const Entry &rhs) const {
      if (lhs.first == rhs.first) {
        // make sure that nullptr is the biggest
        if (lhs.second == nullptr) {
          return false;
        }
        if (rhs.second == nullptr) {
          return true;
        }
        return lhs.second.get() < rhs.second.get();
      }
      return lhs.first < rhs.first;
    }
  };
  using ActiveEntry = std::shared_ptr<Timer>;

  EventLoop *loop_;
  const int timerfd_;
  Channel channel_;
  // Timer set sorted by 1.TimeStamp 2.Timer shared_ptr
  std::set<Entry, Compare> timers_;
  // for cancel purpose
  std::set<ActiveEntry> active_timers_;
  // consider the case that the timer is canceled in its own callback
  bool is_doing_timer_callback_{false};
  std::set<ActiveEntry> cancelingTimers_;

  std::vector<Entry> getExpired(TimeStamp now);
  void reset(const std::vector<Entry> &expired, TimeStamp now);
  void resetTimerfd(TimeStamp nextExpiration);
  bool insert(const std::shared_ptr<Timer> &timer);

 public:
  DISABLE_COPYING_AND_MOVING(TimerQueue);
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  std::shared_ptr<Timer> addTimer(TimeStamp when, double interval, std::function<void()> cb);
  void cancelTimer(const std::shared_ptr<Timer> &timer);
};