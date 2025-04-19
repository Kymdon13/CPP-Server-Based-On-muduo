#include "Timer.h"

Timer::Timer(TimeStamp timestamp, double interval, const std::function<void()>& cb)
    : expiration_(timestamp),
      cb_(cb),
      isInterval_(interval > 0.),
      interval_(static_cast<time_t>(interval * SECOND2MICROSECOND)) {}

void Timer::restart(TimeStamp now) {
  if (isInterval_) {
    expiration_.setTime(now.getTime() + interval_);
  } else {
    // make it invalid
    expiration_.setTime(0);
  }
}

void Timer::run() const { cb_(); }

TimeStamp Timer::expiration() const { return expiration_; }

bool Timer::isInterval() const { return isInterval_; }
