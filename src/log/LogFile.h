#pragma once

#include <sys/time.h>

#include <memory>

#include "base/FileUtil.h"
#include "timer/TimeStamp.h"

class LogFile {
 private:
  const std::string dir_;
  const size_t rollSize_;
  const int checkEveryN_;
  time_t lastRoll_;
  // time_t lastFlush_;

  int count_;
  time_t periodStartingPoint_;
  const time_t period_;

  std::unique_ptr<AppendFile> file_;
  static std::string getLogFileName(TimeStamp& now);

 public:
  /// @brief write log to file
  /// @param path path in which the log file will be created
  /// @param rollSize size of the log file (in bytes, not strictly aligned)
  /// @param checkEveryN if we have written checkEveryN times and the writtenBytes still less than rollSize, we will
  /// flush the file
  /// @param period period in seconds
  LogFile(const std::string& path,
          size_t rollSize = 100 * 1024,  // 100KB
          int checkEveryN = 100, time_t period = 60 * 60 * 24);
  ~LogFile() = default;

  void append(const char* logline, int len);
  void flush() { file_->flush(); }
  void rollFile(TimeStamp& now);
};
