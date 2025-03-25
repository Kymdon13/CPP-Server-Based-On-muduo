#pragma once

#include <sys/epoll.h>

#include <memory>
#include <vector>

#include "cppserver-common.h"

class Poller {
 private:
  int epfd_;
  std::unique_ptr<epoll_event[]> events_{nullptr};

 public:
  DISABLE_COPYING_AND_MOVING(Poller);
  Poller();
  ~Poller();

  std::vector<Channel *> Poll(int timeout = -1) const;

  void UpdateChannel(Channel *channel) const;
  void DeleteChannel(Channel *channel) const;
};