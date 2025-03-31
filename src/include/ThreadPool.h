#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "cppserver-common.h"

class ThreadPool {
 private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex tasks_mutex_;
  std::condition_variable condition_variable_;
  std::atomic<bool> terminated_{false};

 public:
  DISABLE_COPYING_AND_MOVING(ThreadPool);
  explicit ThreadPool(size_t size = std::thread::hardware_concurrency());
  ~ThreadPool();

  template <class F, class... Args>
  // std::result_of is used to determine this return type at compile time.
  auto Add(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
};

// add() takes a callable object f and its arguments args as input.
// It returns a std::future that will hold the result of the asynchronous
// execution of f with args.
template <class F, class... Args>
auto ThreadPool::Add(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
  using return_type = typename std::result_of<F(Args...)>::type;

  // std::packaged_task is a class template that wraps a callable object (in
  // this case, the result of std::bind) and makes its result available through
  // a std::future.
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      // std::forward is used for perfect forwarding, ensuring that the
      // arguments are passed to f with their original value category (lvalue or
      // rvalue).
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  // get_future() is a method of packaged_task that retrieves the std::future
  // object associated with the packaged_task.
  std::future<return_type> res = task->get_future();

  {
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    if (terminated_) throw std::runtime_error("ThreadPool terminated, add task failed");
    tasks_.emplace([task]() { (*task)(); });
  }
  condition_variable_.notify_one();

  // The function returns the std::future object res. The caller can use this
  // future to wait for the result of the task and retrieve it (or handle any
  // exceptions).
  return res;
}
