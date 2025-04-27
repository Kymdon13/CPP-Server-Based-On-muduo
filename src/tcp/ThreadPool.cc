#include "ThreadPool.h"

#include "Thread.h"

ThreadPool::ThreadPool(EventLoop* loop, int n_threads) : mainReactor_(loop), n_threads_(n_threads) {}

ThreadPool::~ThreadPool() {}

void ThreadPool::init() {
  for (size_t i = 0; i < n_threads_; ++i) {
    threads_.emplace_back(std::make_unique<Thread>());
    loops_.push_back(threads_.back()->startLoop());
  }
}

EventLoop* ThreadPool::getSubReactor() {
  size_t min_load = SIZE_MAX;
  EventLoop* min_loop = nullptr;
  for (auto &loop : loops_) {
    size_t load = loop->load();
    if (load < min_load) {
      min_load = load;
      min_loop = loop.get();
    }
  }
  // if there is not any sub reactors in the loops_, then main reactor will do the work
  if (min_loop == nullptr) {
    return mainReactor_.get();
  } else {
    // add 1 connection to the workload of fetched EventLoop
    min_loop->workload()->connections_.fetch_add(1);
    return min_loop;
  }
}
