#include "Timer.h"

Timer::Timer(TimeStamp timestamp, double interval, const std::function<void()> &cb):
expiration_(timestamp),
cb_(cb),
is_interval_(interval > 0.),
interval_(static_cast<time_t>(interval * SECOND2MICROSECOND)) {}

void Timer::Restart(TimeStamp now) {
    if (is_interval_) {
        expiration_.SetTime(now.GetTime() + interval_);
    } else {
        // make it invalid
        expiration_.SetTime(0);
    }
}

void Timer::Run() const { cb_(); }

TimeStamp Timer::GetExpiration() const { return expiration_; }

bool Timer::IsInterval() const { return is_interval_; }
