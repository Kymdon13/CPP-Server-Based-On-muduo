#include "ThreadPool.h"
#include "Thread.h"

ThreadPool::ThreadPool(EventLoop *loop, int n_threads) : main_reactor_(loop), n_threads_(n_threads) {}

ThreadPool::~ThreadPool() {}

void ThreadPool::Init() {
  for (size_t i = 0; i < n_threads_; ++i) {
    threads_.emplace_back(std::make_unique<Thread>());
    loops_.push_back(threads_.back()->StartLoop());
  }
}

EventLoop *ThreadPool::GetSubReactor() {
  if (!loops_.empty()) {
    ++which_sub_reactor_;
    which_sub_reactor_ %= loops_.size();
    return loops_[which_sub_reactor_].get();
  }
  // if there is not any sub reactors in the loops_, then main reactor will do the work
  return main_reactor_.get();
}
