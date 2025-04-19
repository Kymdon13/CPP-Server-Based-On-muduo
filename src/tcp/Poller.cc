#include "Poller.h"

#include <unistd.h>

#include <cstring>
#include <memory>
#include <vector>

#include "Channel.h"
#include "base/Exception.h"
#include "log/Logger.h"

#define MAX_EVENTS 1000

Poller::Poller() {
  epollfd_ = epoll_create1(0);  // create system epll
  if (-1 == epollfd_) LOG_SYSERR << "epoll_create1() error";
  events_ = std::make_unique<epoll_event[]>(MAX_EVENTS);  // array for struct epoll_event
  memset(events_.get(), 0, sizeof(epoll_event) * MAX_EVENTS);
}

Poller::~Poller() {
  if (epollfd_ != -1) {
    ::close(epollfd_);
  }
}

std::vector<Channel*> Poller::Poll(int timeout) const {
  std::vector<Channel*> ready_channels;
  int nfds = 0;
  do {
    nfds = epoll_wait(epollfd_, events_.get(), MAX_EVENTS, timeout);  // get ready events
  } while (nfds == -1 && errno == EINTR);
  if (-1 == nfds) LOG_SYSERR << "epoll_wait() error";
  // fetch Channels from epoll_events and put them in a vector
  for (int i = 0; i < nfds; ++i) {
    Channel* ch = (Channel*)events_[i].data.ptr;
    event_t ev = events_[i].events;
    ch->setReadyEvents(ev);
    ready_channels.emplace_back(ch);
  }
  return ready_channels;
}

void Poller::updateChannel(Channel* channel) const {
  int sockfd = channel->fd();
  struct epoll_event ev {};
  ev.data.ptr = channel;
  ev.events = channel->listenEvent();
  if (!channel->isInEpoll()) {
    if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &ev)) {
      LOG_SYSERR << "epoll_ctl(EPOLL_CTL_ADD) error";
      return;
    }
    channel->setInEpoll(true);
  } else {
    if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_MOD, sockfd, &ev)) {
      LOG_SYSERR << "epoll_ctl(EPOLL_CTL_MOD) error";
    }
  }
}

void Poller::deleteChannel(Channel* channel) const {
  int sockfd = channel->fd();
  if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_DEL, sockfd, nullptr)) {
    LOG_SYSERR << "epoll_ctl(EPOLL_CTL_DEL) error";
    return;
  }
  channel->setInEpoll(false);
}
