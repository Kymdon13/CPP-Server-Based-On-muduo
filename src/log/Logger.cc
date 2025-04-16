#include "Logger.h"

#include <sys/time.h>

#include "timer/TimeStamp.h"

const char *LogLevelName[static_cast<size_t>(Logger::LogLevel::NUM_LOG_LEVELS)] = {
    "[TRACE] ", "[DEBUG] ", "[INFO] ", "[WARN] ", "[ERROR] ", "[FATAL] ",
};

// TODO(wzy): do we need to use thread_local here? different threads will have different Logger objects
thread_local char t_time[64];
thread_local time_t t_lastMicrosecond = 0;

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v) {
  s.append(v.filename_, v.size_);
  return s;
}

void defaultOutput(const char *msg, int len) {
  size_t n = fwrite(msg, 1, len, stdout);
  (void)n;
}

void defaultFlush() { fflush(stdout); }

Logger::LogLevel g_logLevel = Logger::LogLevel::TRACE;
// output to stdout by default
Logger::outputFunc g_output = defaultOutput;
Logger::flushFunc g_flushBeforeAbort = defaultFlush;

Logger::Wrapper::Wrapper(LogLevel level, const SourceFile &file, int line)
    : time_(TimeStamp::now()), stream_(), level_(level), file_(file), line_(line) {
  formatTime();
  CurrentThread::gettid();
  // 2. thread info
  stream_ << CurrentThread::tidString() << ' ';
  // 3. log level info
  stream_ << LogLevelName[static_cast<size_t>(level)];
  // 5. filename & line number
  stream_ << '(' << file_ << ':' << line_ << ") ";
}
void Logger::Wrapper::formatTime() {
  // if multiple messages loaded during the lifetime of the Logger, reuse t_time
  if (t_lastMicrosecond != time_.getTime()) {
    t_lastMicrosecond = time_.getTime();
    std::string time_str = time_.formattedString();
    memcpy(t_time, time_str.c_str(), time_str.size());
  }
  // 1. time info
  stream_ << t_time << ' ';
}

Logger::Logger(SourceFile file, int line) : wrapper_(LogLevel::INFO, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level) : wrapper_(level, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func) : wrapper_(level, file, line) {
  // 4. function info
  wrapper_.stream_ << '[' << func << "] ";
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : wrapper_(toAbort ? LogLevel::FATAL : LogLevel::ERROR, file, line) {}

Logger::~Logger() {
  // 6. line feed
  wrapper_.finish();
  // get the Logger::stream_ and write it to the output function
  const LogStream::Buffer &buf(stream().buffer());
  g_output(buf.data(), buf.length());
  // if the log level is FATAL, flush the log and abort the program
  if (wrapper_.level_ == LogLevel::FATAL) {
    g_flushBeforeAbort();
    abort();
  }
}

Logger::LogLevel Logger::logLevel() { return g_logLevel; }
void Logger::setLogLevel(LogLevel level) { g_logLevel = level; }

void Logger::setOutput(outputFunc fn) { g_output = fn; }
void Logger::setFlush(flushFunc fn) { g_flushBeforeAbort = fn; }
