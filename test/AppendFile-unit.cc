#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "base/FileUtil.h"

int main() {
  std::string file = "log/1/test1.log";
  FileUtil::AppendFile a(file);
  a.append("hello world\n", 12);
  a.flush();
  for (int i = 0; i < 1000000; ++i) {
    a.append("hello world\n", 12);
  }
  a.flush();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // remove the log file
  std::filesystem::path filepath(file);
  if (std::filesystem::exists(filepath)) {
    std::filesystem::remove(filepath);
    std::cout << "test.log removed" << std::endl;
  } else {
    std::cout << "test.log does not exist" << std::endl;
  }
  return 0;
}
