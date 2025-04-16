#pragma once

#include <sys/epoll.h>

#include <memory>
#include <vector>

#include "base/common.h"

class Channel;

class Poller {
 private:
  int epollfd_;
  std::unique_ptr<epoll_event[]> events_{nullptr};

 public:
  DISABLE_COPYING_AND_MOVING(Poller);
  Poller();
  ~Poller();

  std::vector<Channel *> Poll(int timeout = -1) const;

  void updateChannel(Channel *channel) const;
  void deleteChannel(Channel *channel) const;
};
