#pragma once

#include <sys/time.h>

#include <functional>

#include "base/cppserver-common.h"
#include "TimeStamp.h"

class Timer {
private:
    TimeStamp expiration_;
    std::function<void()> cb_;
    bool is_interval_;
    time_t interval_;
public:
    DISABLE_COPYING_AND_MOVING(Timer);
    Timer(TimeStamp timestamp, double interval, const std::function<void()> &cb);

    void Restart(TimeStamp now);

    void Run() const;

    TimeStamp GetExpiration() const;

    bool IsInterval() const;
};
