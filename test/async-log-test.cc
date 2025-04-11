#include <iostream>

#include "base/FileUtil.h"
#include "log/AsyncLogging.h"
#include "log/LogStream.h"
#include "log/Logger.h"

int main() {
  std::shared_ptr<AsyncLogging> asyncLog = AsyncLogging::Init();
  asyncLog->start();

  /**
   * Logger
   */
  Logger::setLogLevel(Logger::LogLevel::TRACE);

  for (int i = 0; i < 5000; ++i) {
    LOG_TRACE << "hello world";
    LOG_DEBUG << "hello world";
    LOG_INFO << "hello world";
    LOG_WARN << "hello world";
    LOG_ERROR << "hello world";
  }
  //   LOG_FATAL << "hello world";

  std::cout << "end of main()" << std::endl;

  return 0;
}