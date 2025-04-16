#include <sys/stat.h>
#include <iostream>

int main() {
  struct stat file_stat;
  std::cout << stat("./test.cpp", &file_stat) << std::endl;
  std::cout << file_stat.st_ino << std::endl;
}
