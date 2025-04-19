#include "TimeStamp.h"

#include <stdio.h>

// constructs an invalid TimeStamp
TimeStamp::TimeStamp() : time_(0) {}

TimeStamp::TimeStamp(time_t time) : time_(time) {}

TimeStamp TimeStamp::operator+(const double& time) {
  time_t delta = static_cast<time_t>(time * SECOND2MICROSECOND);
  return TimeStamp(time_ += delta);
}

std::string TimeStamp::formattedString(const char* format) const {
  char buf[64] = {0};
  time_t seconds = static_cast<time_t>(time_ / SECOND2MICROSECOND);
  struct tm tm_time;
  localtime_r(&seconds, &tm_time);
  time_t micro_seconds = static_cast<time_t>(time_ % SECOND2MICROSECOND);
  snprintf(buf, sizeof(buf), format, tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour,
           tm_time.tm_min, tm_time.tm_sec, micro_seconds);
  return buf;
}
