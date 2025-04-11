#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "base/CurrentThread.h"
#include "log/LogFile.h"

int main() {
  std::string dir = "log/1/";
  LogFile log(dir);
  for (int j = 0; j < 5; ++j) {
    for (int i = 0; i < 1000; ++i) {
      log.append("helloworl\n", 10);
    }
    sleep(1);
  }
  std::cout << CurrentThread::gettid() << std::endl;
  return 0;
}
