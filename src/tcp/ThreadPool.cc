#include "ThreadPool.h"
#include "Thread.h"

ThreadPool::ThreadPool(EventLoop *loop, int n_threads) : mainReactor_(loop), n_threads_(n_threads) {}

ThreadPool::~ThreadPool() {}

void ThreadPool::init() {
  for (size_t i = 0; i < n_threads_; ++i) {
    threads_.emplace_back(std::make_unique<Thread>());
    loops_.push_back(threads_.back()->startLoop());
  }
}

EventLoop *ThreadPool::getSubReactor() {
  if (!loops_.empty()) {
    ++whichSubReactor_;
    whichSubReactor_ %= loops_.size();
    return loops_[whichSubReactor_].get();
  }
  // if there is not any sub reactors in the loops_, then main reactor will do the work
  return mainReactor_.get();
}
