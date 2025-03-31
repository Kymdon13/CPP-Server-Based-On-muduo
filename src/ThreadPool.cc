#include "include/ThreadPool.h"

ThreadPool::ThreadPool(size_t size) {
  for (size_t i = 0; i < size; ++i) {
    workers_.emplace_back(std::thread([this]() {  // pass 'this' to access data member
      while (true) {                              // waiting task's loop
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(tasks_mutex_);
          condition_variable_.wait(lock,
                                   [this]() {  // wait for another thread to fetch a task
                                     return terminated_ || !tasks_.empty();
                                   });
          // if we are calling dtor and there is no task, exit this thread
          // permanently
          if (terminated_ && tasks_.empty()) return;
          // otherwise fetch a task
          task = tasks_.front();
          tasks_.pop();
        }
        task();  // exec the task, after the task is done, go back to endless
                 // loop again
      }
    }));
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(tasks_mutex_);  // prevent task adding after terminated_ is true
    terminated_ = true;
  }
  // notify all  in the ThreadPool
  condition_variable_.notify_all();
  // before we release resources, wait for each thread to complete its task
  for (auto &thread : workers_)
    if (thread.joinable()) thread.join();
}
