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
#include "CurrentThread.h"
#include "Exception.h"
#include "Poller.h"

void EventLoop::loop_close_wait_list_() {
  // fetch callback from close_wait_list_ so sub threads can add new items when main thread is handling tmp_list
  std::vector<std::function<void()>> tmp_list;
  {
    std::unique_lock<std::mutex> lock(close_wait_list_mutex_);
    tmp_list.swap(close_wait_list_);
  }
  for (auto &cb : tmp_list) {
    cb();
  }
}

EventLoop::EventLoop() {
  poller_ = std::make_unique<Poller>();

  // create eventfd
  wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  // create Channel with eventfd and register it into current EventLoop
  wakeup_channel_ = std::make_unique<Channel>(wakeup_fd_, this);
  // set
  wakeup_channel_->SetReadCallback([this]() {
    uint64_t val;
    ssize_t bytes_read = ::read(wakeup_fd_, &val, sizeof(val));
    WarnIf(bytes_read != sizeof(val), "EventLoop::on_eventfd_: bytes_read != sizeof(val)");
  });
  wakeup_channel_->FlushEvent();
}

EventLoop::~EventLoop() {}

void EventLoop::Loop() {
  tid_ = CurrentThread::gettid();
  while (true) {
    for (Channel *ready_channel : poller_->Poll()) {
      ready_channel->HandleEvent();
    }
    loop_close_wait_list_();
  }
}

void EventLoop::UpdateChannel(Channel *channel) const { poller_->UpdateChannel(channel); }

void EventLoop::DeleteChannel(Channel *channel) const { poller_->DeleteChannel(channel); }

bool EventLoop::IsMainThread() { return tid_ == CurrentThread::gettid(); }

void EventLoop::CallOrQueue(std::function<void()> cb) {
  if (IsMainThread()) {
    cb();
  } else {
    // add to the close_wait_list_
    {
      std::unique_lock<std::mutex> lock(close_wait_list_mutex_);
      close_wait_list_.emplace_back(std::move(cb));
    }
    // inform the eventfd (wakeup_fd_)
    uint64_t val = 1;
    ssize_t bytes_write = ::write(wakeup_fd_, &val, sizeof(val));
    WarnIf(bytes_write != sizeof(val), "EventLoop::CallOrQueue: bytes_write != sizeof(val)");
  }
}
