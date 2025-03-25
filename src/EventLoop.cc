#include "include/EventLoop.h"

#include <vector>

#include "include/Channel.h"
#include "include/Poller.h"

EventLoop::EventLoop() { poller_ = std::make_unique<Poller>(); }

EventLoop::~EventLoop() {}

void EventLoop::Loop() const {
  while (true) {
    for (Channel *ready_channel : poller_->Poll()) {
      ready_channel->HandleEvent();
    }
  }
}

void EventLoop::UpdateChannel(Channel *channel) const { poller_->UpdateChannel(channel); }

void EventLoop::DeleteChannel(Channel *channel) const { poller_->DeleteChannel(channel); }
