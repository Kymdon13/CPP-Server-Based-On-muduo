#include "TimerQueue.h"

#include <string.h>
#include <unistd.h>

#include <cassert>

#include "Timer.h"
#include "base/Exception.h"
#include "log/Logger.h"
#include "tcp/EventLoop.h"

bool TimerQueue::insert(const std::shared_ptr<Timer>& timer) {
  bool earliest = false;
  TimeStamp when = timer->expiration();
  auto it = timers_.begin();
  if (timers_.empty() || when < it->first) {
    earliest = true;
  }
  // insert the timer into the timers_ and active_timers_ set, set.insert() will return pair<iterator, bool>, if the
  // timer already exists the second will be false
  timers_.insert(Entry(when, timer));
  active_timers_.insert(timer);
  return earliest;
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(TimeStamp now) {
  std::vector<Entry> expired;
  // filter the expired timers
  auto it = timers_.lower_bound(Entry(now, nullptr));
  // copy the expired timers to the expired vector
  std::copy(timers_.begin(), it, std::back_inserter(expired));
  // erase the expired timers from the timers_ set
  timers_.erase(timers_.begin(), it);
  // erase the expired timers from the active_timers_ set
  for (auto& entry : expired) {
    active_timers_.erase(entry.second);
  }
  return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, TimeStamp now) {
  // reset the timers
  for (auto& entry : expired) {
    auto timer = entry.second;
    // filter out the timers in the cancelingTimers_
    if (timer->isInterval() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
      // set the expiration_
      timer->restart(now);
      // reinsert the timer to the timers_ and active_timers_ set
      insert(timer);
    }
  }
  // set the timerfd to the next expiration time
  TimeStamp nextExpiration;
  if (!timers_.empty()) {
    nextExpiration = timers_.begin()->first;
    if (nextExpiration.isValid()) {
      resetTimerfd(nextExpiration);
    }
  }
}

void TimerQueue::resetTimerfd(TimeStamp nextExpiration) {
  struct itimerspec new_value;
  bzero(&new_value, sizeof(new_value));
  time_t micro = nextExpiration.getTime() - TimeStamp::now().getTime();
  // make sure the timerfd is set to at least 100 microseconds
  if (micro < 100) {
    micro = 100;
  }
  new_value.it_value.tv_sec = static_cast<time_t>(micro / SECOND2MICROSECOND);
  new_value.it_value.tv_nsec = static_cast<long>((micro % SECOND2MICROSECOND) * 1000);
  // set the timerfd to the next expiration time
  int ret = timerfd_settime(timerfd_, 0, &new_value, NULL);
  if (ret < 0) {
    // handle error
    perror("Failed to set timerfd");
  }
}

int createTimerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0) {
    // handle error
    perror("Failed to create timerfd");
  }
  return timerfd;
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop), timerfd_(createTimerfd()), channel_(timerfd_, loop, true, false, false, false) {
  channel_.setReadCallback([this]() {
    TimeStamp now = TimeStamp::now();
    // read the timerfd
    uint64_t howmany = 0;
    ssize_t n = read(timerfd_, &howmany, sizeof(howmany));
    if (n != sizeof(howmany)) {
      // handle error
      perror("Failed to read timerfd");
    }
    // get the expired timers
    auto expired = getExpired(now);
    // call the timer callbacks
    is_doing_timer_callback_ = true;
    cancelingTimers_.clear();
    for (auto& entry : expired) {
      entry.second->run();
    }
    is_doing_timer_callback_ = false;
    // reset the timers
    reset(expired, now);
  });
  // remember always enable the channel after setting the callback
  channel_.flushEvent();
}

TimerQueue::~TimerQueue() {
  // remove the channel
  channel_.disableAll();
  channel_.removeSelf();
  // close the timerfd
  ::close(timerfd_);
}

std::shared_ptr<Timer> TimerQueue::addTimer(TimeStamp when, double interval, std::function<void()> cb) {
  std::shared_ptr<Timer> timer = std::make_shared<Timer>(when, interval, cb);
  // enqueue the add operation to the pendingFunctors_
  loop_->callOrQueue([this, timer]() {
    // add the timer to the timers_ and active_timers_ set
    bool earliest = insert(timer);
    if (earliest) {
      resetTimerfd(timer->expiration());
    }
  });
  return timer;  // for the user to cancel the timer in the future
}

void TimerQueue::cancelTimer(const std::shared_ptr<Timer>& timer) {
  loop_->callOrQueue([this, timer]() {
    // remove the timer from the timers_ and active_timers_ set
    auto it = active_timers_.find(timer);
    if (it != active_timers_.end()) {
      timers_.erase(Entry(timer->expiration(), timer));
      active_timers_.erase(it);
    } else if (is_doing_timer_callback_) {
      cancelingTimers_.insert(timer);
    } else {
      // handle error
      LOG_ERROR << "TimerQueue::cancelTimer: timer not found && not doing timer's callback";
    }
  });
}
