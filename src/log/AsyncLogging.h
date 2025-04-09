#include <memory>

#include "base/cppserver-common.h"

class AsyncLogging {
    private:
     void threadFunc();
     void flush();

     const std::string basename_;
     int flushInterval_;
     bool running_;
 public:
    DISABLE_COPYING_AND_MOVING(AsyncLogging);

  AsyncLogging(const std::string &basename, int flushInterval = 3);
  ~AsyncLogging() = default;

  void append(const char *logline, int len);
  void start();
  void stop();

};