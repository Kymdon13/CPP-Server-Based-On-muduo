#pragma once

#include <string.h>

#include <functional>

#include "LogStream.h"
#include "base/CurrentThread.h"
#include "timer/TimeStamp.h"

class Logger {
 public:
  /**
   * internal class
   */
  enum class LogLevel : uint8_t { TRACE = 0, DEBUG, INFO, WARN, ERROR, FATAL, NUM_LOG_LEVELS };

  // a simple struct to store the filename and its size
  typedef struct SourceFile {
    const char* filename_;
    int size_;
    SourceFile(const char* filename) : filename_(filename) {
      // last_slash points to the last '/' in the filename
      const char* last_slash = strrchr(filename, '/');
      if (last_slash) {  // if find '/'
        filename_ = last_slash + 1;
      }
      size_ = static_cast<int>(strlen(filename_));
    }
  } SourceFile;

  // a wrapper to output formatted log messages
  class Wrapper {
   public:
    Wrapper(LogLevel level, const SourceFile& file, int line);

    void formatTime();
    void finish() { stream_ << '\n'; }

    TimeStamp time_;
    LogStream stream_;
    LogLevel level_;
    SourceFile file_;
    int line_;
  };

 private:
  Wrapper wrapper_;

 public:
  typedef std::function<void(const char* msg, int len)> outputFunc;
  typedef std::function<void()> flushFunc;

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return wrapper_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  static void setOutput(outputFunc fn);
  static void setFlush(flushFunc fn);
};

// you can use Logger::setLogLevel() to set the log level, those below the level will not be printed
// the default log level is INFO
// WARN, ERROR, FATAL will always be printed
// since the macro do not store the Logger into a variable, the destructor will be called once
#define LOG_TRACE                                    \
  if (Logger::logLevel() <= Logger::LogLevel::TRACE) \
  Logger(__FILE__, __LINE__, Logger::LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG                                    \
  if (Logger::logLevel() <= Logger::LogLevel::DEBUG) \
  Logger(__FILE__, __LINE__, Logger::LogLevel::DEBUG, __func__).stream()
#define LOG_INFO \
  if (Logger::logLevel() <= Logger::LogLevel::INFO) Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::LogLevel::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::LogLevel::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::LogLevel::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()
