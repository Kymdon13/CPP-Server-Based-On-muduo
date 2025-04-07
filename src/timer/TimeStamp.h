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

  bool operator<(const TimeStamp &rhs) const;
  bool operator<=(const TimeStamp &rhs) const;
  bool operator==(const TimeStamp &rhs) const;

  TimeStamp operator+(const double &time);

  std::string ToFormattedString() const;

  time_t GetTime() const;
  void SetTime(time_t time);

  bool IsValid() const { return time_ > 0; }

  static inline TimeStamp Now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return TimeStamp(tv.tv_sec * SECOND2MICROSECOND + tv.tv_usec);
  }
};
