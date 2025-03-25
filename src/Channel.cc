#include "include/Channel.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <utility>

#include "EventLoop.h"
#include "Exception.h"

void Channel::updateEvent(event_t event, bool enable) {
  if (enable) {
    listen_event_ |= event;
  } else {
    listen_event_ &= ~event;
  }
  flushable_ = true;
}

void Channel::flushEvent() {
  if (flushable_) {
    loop_->UpdateChannel(this);
    flushable_ = false;
  }
}

Channel::Channel(int fd, EventLoop *loop, bool enableReading, bool enableWriting, bool useET)
    : fd_(fd), loop_(loop), listen_event_(0), ready_event_(0) {
  // set listen_event_
  if (enableReading) {
    updateEvent(EPOLLIN | EPOLLPRI, true);
  }
  if (enableWriting) {
    updateEvent(EPOLLOUT, true);
  }
  if (useET) {
    updateEvent(EPOLLET, true);
  }
  flushEvent();
}

Channel::~Channel() {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

void Channel::HandleEvent() const {
  if (ready_event_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (read_callback_) {
      read_callback_();
    } else {
      WarnIf(true, "register EPOLLIN but no read_callback_ is registered");
    }
  }
  if (ready_event_ & EPOLLOUT) {
    if (write_callback_) {
      write_callback_();
    } else {
      WarnIf(true, "register EPOLLOUT but no write_callback_ is registered");
    }
  }
}

void Channel::EnableReading() {
  updateEvent(EPOLLIN | EPOLLPRI, true);
  flushEvent();
}

void Channel::DisableReading() {
  updateEvent(EPOLLIN | EPOLLPRI, false);
  flushEvent();
}

void Channel::EnableWriting() {
  updateEvent(EPOLLOUT, true);
  flushEvent();
}

void Channel::DisableWriting() {
  updateEvent(EPOLLOUT, false);
  flushEvent();
}

int Channel::GetFD() const { return fd_; }

event_t Channel::GetListenEvent() const { return listen_event_; }

event_t Channel::GetReadyEvent() const { return ready_event_; }

bool Channel::IsInEpoll() const { return in_epoll_; }

void Channel::SetInEpoll(bool in) { in_epoll_ = in; }

void Channel::SetReadyEvents(event_t ev) { ready_event_ = ev; }

void Channel::SetWriteCallback(std::function<void()> const &callback) { read_callback_ = std::move(callback); }

void Channel::SetReadCallback(std::function<void()> const &callback) { write_callback_ = std::move(callback); }
