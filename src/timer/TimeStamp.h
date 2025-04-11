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

  bool operator<(const TimeStamp &rhs) const { return time_ < rhs.GetTime(); }
  bool operator<=(const TimeStamp &rhs) const { return time_ <= rhs.GetTime(); }
  bool operator==(const TimeStamp &rhs) const { return time_ == rhs.GetTime(); }

  TimeStamp operator+(const double &time);

  std::string ToFormattedString(const char *format = "%4d-%02d-%02d %02d:%02d:%02d.%06d") const;

  time_t GetTime() const { return time_; };
  time_t GetSecond() const { return static_cast<time_t>(time_ / SECOND2MICROSECOND); }
  void SetTime(time_t time) { time_ = time; };

  bool IsValid() const { return time_ > 0; }

  static inline TimeStamp Now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return TimeStamp(tv.tv_sec * SECOND2MICROSECOND + tv.tv_usec);
  }
};
