#pragma once

#include <sys/time.h>

#include <functional>

#include "TimeStamp.h"
#include "base/common.h"

class Timer {
 private:
  TimeStamp expiration_;
  std::function<void()> cb_;
  bool isInterval_;
  time_t interval_;

 public:
  DISABLE_COPYING_AND_MOVING(Timer);
  Timer(TimeStamp timestamp, double interval, const std::function<void()> &cb);

  void restart(TimeStamp now);

  void run() const;

  TimeStamp expiration() const;

  bool isInterval() const;
};
