#pragma once

#include <sys/time.h>

#include <string>

#define SECOND2MICROSECOND 1000000

class TimeStamp {
 private:
  // time in micro seconds
  time_t time_;

 public:
  TimeStamp();
  explicit TimeStamp(time_t time);

  bool operator<(const TimeStamp& rhs) const { return time_ < rhs.getTime(); }
  bool operator<=(const TimeStamp& rhs) const { return time_ <= rhs.getTime(); }
  bool operator==(const TimeStamp& rhs) const { return time_ == rhs.getTime(); }

  TimeStamp operator+(const double& time);

  std::string formattedString(const char* format = "%4d-%02d-%02d %02d:%02d:%02d.%06d") const;

  time_t getTime() const { return time_; };
  time_t getSecond() const { return static_cast<time_t>(time_ / SECOND2MICROSECOND); }
  void setTime(time_t time) { time_ = time; };

  bool isValid() const { return time_ > 0; }

  static inline TimeStamp now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return TimeStamp(tv.tv_sec * SECOND2MICROSECOND + tv.tv_usec);
  }
};
