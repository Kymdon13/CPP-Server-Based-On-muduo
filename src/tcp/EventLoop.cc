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
  ssize_t bytes_write = ::write(wakefd_, &val, sizeof(val));
  if (bytes_write != sizeof(val)) {
    LOG_ERROR << "EventLoop::wake, bytes_write != sizeof(val)";
  }
}

EventLoop::EventLoop()
    : tid_(CurrentThread::gettid()),
      poller_(std::make_unique<Poller>()),
      timerQueue_(std::make_unique<TimerQueue>(this)) {
  // create eventfd
  wakefd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  // create Channel with eventfd and register it into current EventLoop
  wakeChannel_ = std::make_unique<Channel>(wakefd_, this, true, false, true, false);
  // set
  wakeChannel_->setReadCallback([this]() {
    uint64_t val;
    ssize_t bytes_read = ::read(wakefd_, &val, sizeof(val));
    if (bytes_read != sizeof(val)) {
      LOG_ERROR << "EventLoop::EventLoop, bytes_read != sizeof(val)";
    }
  });
  wakeChannel_->flushEvent();
}

EventLoop::~EventLoop() {}

void EventLoop::quit() {
  callOrQueue([this]() { quit_ = true; });
  // wake up the loop
  wake();
}

void EventLoop::loop() {
  while (!quit_) {
    for (Channel *ready_channel : poller_->Poll()) {
      ready_channel->handleEvent();
    }
    doPendingFunctors();
  }
}

void EventLoop::updateChannel(Channel *channel) const { poller_->updateChannel(channel); }

void EventLoop::deleteChannel(Channel *channel) const { poller_->deleteChannel(channel); }

bool EventLoop::isLocalThread() { return tid_ == CurrentThread::gettid(); }

void EventLoop::callOrQueue(std::function<void()> cb) {
  if (isLocalThread()) {  //
    cb();
  } else {
    // add to the pendingFunctors_
    {
      std::unique_lock<std::mutex> lock(pendingFunctorsMutex_);
      pendingFunctors_.emplace_back(std::move(cb));
    }
    // inform the eventfd (wakefd_)
    wake();
  }
}

std::shared_ptr<Timer> EventLoop::runAt(TimeStamp time, std::function<void()> cb) {
  return timerQueue_->addTimer(time, 0., cb);
}

std::shared_ptr<Timer> EventLoop::runAfter(double delay, std::function<void()> cb) {
  return timerQueue_->addTimer(TimeStamp::now() + delay, 0., cb);
}

std::shared_ptr<Timer> EventLoop::runEvery(double interval, std::function<void()> cb) {
  return timerQueue_->addTimer(TimeStamp::now() + interval, interval, cb);
}

void EventLoop::canelTimer(const std::shared_ptr<Timer> &timer) { timerQueue_->cancelTimer(timer); }
