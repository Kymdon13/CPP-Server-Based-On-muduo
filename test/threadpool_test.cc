#include <iostream>
#include <string>

#include "../src/include/ThreadPool.h"

int share = 0;

void increment() { share = share + 1; }

// Basic functionality tests
int main() {
  ThreadPool *pool = new ThreadPool();
  std::function<void()> func = std::bind(increment);
  for (int i = 0; i < 10000; ++i) {
    pool->Add(func);
  }
  std::cout << share << std::endl;
  delete pool;
  return 0;
}
