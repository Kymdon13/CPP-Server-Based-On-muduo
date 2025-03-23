#include <iostream>
#include <string>
#include "../src/ThreadPool.h"

int share = 0;

void increment() {
    share = share + 1;
}

// Basic functionality tests
int main(int argc, char const *argv[]) {
    ThreadPool *pool = new ThreadPool();
    std::function<void()> func = std::bind(increment);
    for (int i = 0; i < 10000; ++i) {
        pool->add(func);
    }
    std::cout << share << std::endl;
    delete pool;
    return 0;
}
