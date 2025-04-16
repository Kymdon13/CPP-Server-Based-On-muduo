#include "EventLoop.h"

#include <stdio.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cstdio>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "Channel.h"
#include "Poller.h"
#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "log/Logger.h"
#include "timer/Timer.h"
#include "timer/TimerQueue.h"

void EventLoop::doPendingFunctors() {
  // fetch callback from pendingFunctors_ so sub threads can add new items when main thread is handling tmp_list
  std::vector<std::function<void()>> tmp_list;
  {
    std::unique_lock<std::mutex> lock(pendingFunctorsMutex_);
    tmp_list.swap(pendingFunctors_);
  }
  for (auto &cb : tmp_list) {
    cb();
  }
}

void EventLoop::wake() {
  uint64_t val = 1;
  ssize_t bytes_write = ::write(wakeup_fd_, &val, sizeof(val));
  if (bytes_write != sizeof(val)) {
    LOG_ERROR << "EventLoop::wake, bytes_write != sizeof(val)";
  }
}

EventLoop::EventLoop()
    : tid_(CurrentThread::gettid()),
      poller_(std::make_unique<Poller>()),
      timer_queue_(std::make_unique<TimerQueue>(this)) {
  // create eventfd
  wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  // create Channel with eventfd and register it into current EventLoop
  wakeup_channel_ = std::make_unique<Channel>(wakeup_fd_, this, true, false, true, false);
  // set
  wakeup_channel_->SetReadCallback([this]() {
    uint64_t val;
    ssize_t bytes_read = ::read(wakeup_fd_, &val, sizeof(val));
    if (bytes_read != sizeof(val)) {
      LOG_ERROR << "EventLoop::EventLoop, bytes_read != sizeof(val)";
    }
  });
  wakeup_channel_->FlushEvent();
}

EventLoop::~EventLoop() {}

void EventLoop::Quit() {
  CallOrQueue([this]() { quit_ = true; });
  // wake up the loop
  wake();
}

void EventLoop::Loop() {
  while (!quit_) {
    for (Channel *ready_channel : poller_->Poll()) {
      ready_channel->HandleEvent();
    }
    doPendingFunctors();
  }
}

void EventLoop::UpdateChannel(Channel *channel) const { poller_->UpdateChannel(channel); }

void EventLoop::DeleteChannel(Channel *channel) const { poller_->DeleteChannel(channel); }

bool EventLoop::IsLocalThread() { return tid_ == CurrentThread::gettid(); }

void EventLoop::CallOrQueue(std::function<void()> cb) {
  if (IsLocalThread()) {  //
    cb();
  } else {
    // add to the pendingFunctors_
    {
      std::unique_lock<std::mutex> lock(pendingFunctorsMutex_);
      pendingFunctors_.emplace_back(std::move(cb));
    }
    // inform the eventfd (wakeup_fd_)
    wake();
  }
}

std::shared_ptr<Timer> EventLoop::RunAt(TimeStamp time, std::function<void()> cb) {
  return timer_queue_->addTimer(time, 0., cb);
}

std::shared_ptr<Timer> EventLoop::RunAfter(double delay, std::function<void()> cb) {
  return timer_queue_->addTimer(TimeStamp::Now() + delay, 0., cb);
}

std::shared_ptr<Timer> EventLoop::RunEvery(double interval, std::function<void()> cb) {
  return timer_queue_->addTimer(TimeStamp::Now() + interval, interval, cb);
}

void EventLoop::CanelTimer(const std::shared_ptr<Timer> &timer) { timer_queue_->cancelTimer(timer); }
