#include "Poller.h"

#include <unistd.h>

#include <cstring>
#include <memory>
#include <vector>

#include "Channel.h"
#include "Exception.h"

#define MAX_EVENTS 1000

Poller::Poller() {
  epfd_ = epoll_create1(0);  // create system epll
  WarnIf(-1 == epfd_, "Poller::UpdateChannel: epoll_create1() error");
  events_ = std::make_unique<epoll_event[]>(MAX_EVENTS);  // array for struct epoll_event
  memset(events_.get(), 0, sizeof(epoll_event) * MAX_EVENTS);
}

Poller::~Poller() {
  if (epfd_ != -1) {
    ::close(epfd_);
  }
}

std::vector<Channel *> Poller::Poll(int timeout) const {
  std::vector<Channel *> ready_channels;
  int nfds = epoll_wait(epfd_, events_.get(), MAX_EVENTS, timeout);  // get ready events
  WarnIf(-1 == nfds, "epoll_wait() error");
  // fetch Channels from epoll_events and put them in a vector
  for (int i = 0; i < nfds; ++i) {
    Channel *ch = (Channel *)events_[i].data.ptr;
    event_t ev = events_[i].events;
    ch->SetReadyEvents(ev);
    ready_channels.emplace_back(ch);
  }
  return ready_channels;
}

void Poller::UpdateChannel(Channel *channel) const {
  int sockfd = channel->GetFD();
  struct epoll_event ev {};
  ev.data.ptr = channel;
  ev.events = channel->GetListenEvent();
  if (!channel->IsInEpoll()) {
    if (-1 == epoll_ctl(epfd_, EPOLL_CTL_ADD, sockfd, &ev)) {
      WarnIf(true, "Poller::UpdateChannel: epoll_ctl(EPOLL_CTL_ADD) error, will not call Channel::SetInEpoll()");
      return;
    }
    channel->SetInEpoll(true);
  } else {
    WarnIf(-1 == epoll_ctl(epfd_, EPOLL_CTL_MOD, sockfd, &ev), "Poller::UpdateChannel: epoll_ctl(EPOLL_CTL_MOD) error");
  }
}

void Poller::DeleteChannel(Channel *channel) const {
  int sockfd = channel->GetFD();
  if (-1 == epoll_ctl(epfd_, EPOLL_CTL_DEL, sockfd, nullptr)) {
    WarnIf(true,
           "Poller::UpdateChannel: epoll(EPOLL_CTL_DEL) error, which means Channel may still in the system epoll");
    return;
  }
  channel->SetInEpoll(false);
}
