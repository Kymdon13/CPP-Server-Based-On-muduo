#include "Logger.h"

#include <sys/time.h>

#include "timer/TimeStamp.h"

// TODO(wzy): do we need to use thread_local here? different threads will have different Logger objects
thread_local char t_time[64];
thread_local time_t t_lastMicrosecond = 0;

LogLevel g_logLevel = LogLevel::INFO;

inline LogStream &operator<<(LogStream &s, const SourceFile &v) {
  s.append(v.filename_, v.size_);
  return s;
}

void defaultOutput(const char *msg, int len) {
  size_t n = fwrite(msg, 1, len, stdout);
  (void)n;
}

void defaultFlush() { fflush(stdout); }

OutputFunc g_output = defaultOutput;
FlushFunc g_flush = defaultFlush;

void Wrapper::formatTime() {
  // if multiple messages loaded during the lifetime of the Logger, reuse t_time
  if (t_lastMicrosecond != time_.GetTime()) {
    t_lastMicrosecond = time_.GetTime();
    std::string time_str = time_.ToFormattedString();
    memcpy(t_time, time_str.c_str(), time_str.size());
  }
  stream_ << t_time;
}

Logger::Logger(SourceFile file, int line) : wrapper_(LogLevel::INFO, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level) : wrapper_(level, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func) : wrapper_(level, file, line) {
  wrapper_.stream_ << func << "() - ";
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : wrapper_(toAbort ? LogLevel::FATAL : LogLevel::ERROR, file, line) {}

Logger::~Logger() {
  wrapper_.finish();
  // get the Logger::stream_ and write it to the output function
  const LogStream::Buffer &buf(stream().buffer());
  g_output(buf.data(), buf.length());
  // if the log level is FATAL, flush the log and abort the program
  if (wrapper_.level_ == LogLevel::FATAL) {
    g_flush();
    abort();
  }
}
