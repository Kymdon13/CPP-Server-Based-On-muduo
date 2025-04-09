#include <string.h>

#include "LogStream.h"
#include "base/CurrentThread.h"
#include "timer/TimeStamp.h"

enum class LogLevel : uint8_t { TRACE = 0, DEBUG, INFO, WARN, ERROR, FATAL, NUM_LOG_LEVELS };
const char *LogLevelName[static_cast<size_t>(LogLevel::NUM_LOG_LEVELS)] = {
    "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};

typedef void (*OutputFunc)(const char *msg, int len);
typedef void (*FlushFunc)();

extern LogLevel g_logLevel;
extern OutputFunc g_output;
extern FlushFunc g_flush;

// a simple struct to store the filename and its size
typedef struct SourceFile {
  const char *filename_;
  int size_;
  explicit SourceFile(const char *filename) : filename_(filename) {
    // last_slash points to the last '/' in the filename
    const char *last_slash = strrchr(filename, '/');
    if (last_slash) {  // if find '/'
      filename_ = last_slash + 1;
    }
    size_ = static_cast<int>(strlen(filename_));
  }
} SourceFile;

// a wrapper to output formatted log messages
class Wrapper {
 public:
  Wrapper(LogLevel level, const SourceFile &file, int line)
      : time_(TimeStamp::Now()), stream_(), level_(level), file_(file), line_(line) {
    formatTime();
    CurrentThread::gettid();
    stream_ << CurrentThread::tidString();
    stream_ << LogLevelName[static_cast<size_t>(level)];
  }

  void formatTime();
  void finish() { stream_ << " [" << file_.filename_ << ':' << line_ << "]\n"; }

  TimeStamp time_;
  LogStream stream_;
  LogLevel level_;
  SourceFile file_;
  int line_;
};

class Logger {
 private:
  Wrapper wrapper_;

 public:
  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char *func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream &stream() { return wrapper_.stream_; }

  static inline LogLevel logLevel() { return g_logLevel; }
  static inline void setLogLevel(LogLevel level) { g_logLevel = level; }

  static void setOutput(OutputFunc fn) { g_output = fn; }
  static void setFlush(FlushFunc fn) { g_flush = fn; }
};

// you can use Logger::setLogLevel() to set the log level, those below the level will not be printed
// the default log level is INFO
// WARN, ERROR, FATAL will always be printed
// since the macro do not store the Logger into a variable, the destructor will be called once 
#define LOG_TRACE \
  if (Logger::logLevel() <= LogLevel::TRACE) Logger(SourceFile(__FILE__), __LINE__, LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG \
  if (Logger::logLevel() <= LogLevel::DEBUG) Logger(SourceFile(__FILE__), __LINE__, LogLevel::DEBUG, __func__).stream()
#define LOG_INFO \
  if (Logger::logLevel() <= LogLevel::INFO) Logger(SourceFile(__FILE__), __LINE__).stream()
#define LOG_WARN Logger(SourceFile(__FILE__), __LINE__, LogLevel::WARN).stream()
#define LOG_ERROR Logger(SourceFile(__FILE__), __LINE__, LogLevel::ERROR).stream()
#define LOG_FATAL Logger(SourceFile(__FILE__), __LINE__, LogLevel::FATAL).stream()
#define LOG_SYSERR Logger(SourceFile(__FILE__), __LINE__, false).stream()
#define LOG_SYSFATAL Logger(SourceFile(__FILE__), __LINE__, true).stream()
