#include "Channel.h"

#include <unistd.h>

#include <memory>
#include <utility>

#include "EventLoop.h"
#include "base/Exception.h"
#include "log/Logger.h"

void Channel::updateEvent(event_t event, bool enable) {
  if (enable) {
    listenEvent_ |= event;
  } else {
    listenEvent_ &= ~event;
  }
  flushable_ = true;
}

Channel::Channel(int fd, EventLoop* loop, bool enableReading, bool enableWriting, bool useET, bool is_connection)
    : fd_(fd), loop_(loop), listenEvent_(0), readyEvent_(0), isConnection_(is_connection) {
  // set listenEvent_
  if (enableReading) {
    updateEvent(EPOLLIN | EPOLLPRI, true);
  }
  if (enableWriting) {
    updateEvent(EPOLLOUT, true);
  }
  if (useET) {
    updateEvent(EPOLLET, true);
  }
}

Channel::~Channel() {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

void Channel::flushEvent() {
  if (flushable_) {
    loop_->updateChannel(this);
    flushable_ = false;
  }
}

void Channel::handleEvent() const {
  // use_count_lock only exists during the handleEvent method
  std::shared_ptr<TCPConnection> use_count_lock;
  if (isConnection_) {
    use_count_lock = tcpConnection_.lock();
    // TODO(wzy) there can be Acceptor->Channel->handleEvent(), and Acceptor will not set the tcpConnection_ in the
    // Channel, we must find a way to detect whether the caller is an Acceptor
    if (!use_count_lock) {  // if failed to promote the weak_ptr to shared_ptr
      LOG_ERROR << "Channel::handleEvent, failed to exec tcpConnection_.lock(), it's unsafe to continue";
      return;
    }
  }
  if (readyEvent_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (read_callback_) {
      read_callback_();
    } else {
      LOG_WARN << "EPOLLIN registered but no read_callback_ is registered";
    }
  }
  if (readyEvent_ & EPOLLOUT) {
    if (write_callback_) {
      write_callback_();
    } else {
      LOG_WARN << "EPOLLOUT registered but no write_callback_ is registered";
    }
  }
}

void Channel::enableReading() {
  updateEvent(EPOLLIN | EPOLLPRI, true);
  flushEvent();
}
void Channel::disableReading() {
  updateEvent(EPOLLIN | EPOLLPRI, false);
  flushEvent();
}

void Channel::enableWriting() {
  updateEvent(EPOLLOUT, true);
  flushEvent();
}
void Channel::disableWriting() {
  updateEvent(EPOLLOUT, false);
  flushEvent();
}

void Channel::disableAll() {
  listenEvent_ = 0;  // disable all events
  flushable_ = true;
  flushEvent();
}

int Channel::fd() const { return fd_; }

void Channel::removeSelf() {
  if (fd_ != -1) {
    loop_->deleteChannel(this);
    ::close(fd_);
    fd_ = -1;
  }
}

event_t Channel::listenEvent() const { return listenEvent_; }
event_t Channel::readyEvent() const { return readyEvent_; }

bool Channel::isInEpoll() const { return in_epoll_; }
void Channel::setInEpoll(bool in) { in_epoll_ = in; }

void Channel::setReadyEvents(event_t ev) { readyEvent_ = ev; }

void Channel::setReadCallback(std::function<void()> callback) { read_callback_ = std::move(callback); }
void Channel::setWriteCallback(std::function<void()> callback) { write_callback_ = std::move(callback); }

void Channel::setTCPConnection(std::shared_ptr<TCPConnection> conn) { tcpConnection_ = conn; }
