#pragma once

#include <memory>

#include "cppserver-common.h"

class EventLoop {
 private:
  std::unique_ptr<Poller> poller_;

 public:
  DISABLE_COPYING_AND_MOVING(EventLoop);
  EventLoop();
  ~EventLoop();

  void Loop() const;
  void UpdateChannel(Channel *channel) const;
  void DeleteChannel(Channel *channel) const;
};
