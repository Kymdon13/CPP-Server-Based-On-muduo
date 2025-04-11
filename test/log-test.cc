#include <iostream>

#include "base/FileUtil.h"
#include "log/LogStream.h"
#include "log/Logger.h"

int main() {
  LogStream os;
  os << "hello world";
  std::cout << os.buffer().data() << std::endl;
  os.resetBuffer();
  os.bzeroBuffer();

  const char *str[3] = {"hello", "world", "!"};
  os << str[1];
  std::cout << os.buffer().data() << std::endl;
  os.resetBuffer();
  os.bzeroBuffer();

  os << 11;
  std::cout << os.buffer().data() << std::endl;
  os.resetBuffer();
  os.bzeroBuffer();

  os << 0.1;
  std::cout << os.buffer().data() << std::endl;
  os.resetBuffer();
  os.bzeroBuffer();

  /**
   * SourceFile
   */
  Logger::SourceFile file(__FILE__);
  std::cout << file.filename_ << std::endl;
  std::cout << __FILE__ << std::endl;

  TimeStamp time = TimeStamp::Now();
  char time_buf[64] = {0};
  std::string time_str = time.ToFormattedString();
  memcpy(time_buf, time_str.c_str(), time_str.size());
  std::cout << strlen(time_buf) << "|" << time_buf << std::endl;

  /**
   * Logger
   */
  Logger::setLogLevel(Logger::LogLevel::TRACE);
  LOG_TRACE << "hello world";
  LOG_DEBUG << "hello world";
  LOG_INFO << "hello world";
  LOG_WARN << "hello world";
  LOG_ERROR << "hello world";

  std::cout << "end of main()" << std::endl;

  return 0;
}