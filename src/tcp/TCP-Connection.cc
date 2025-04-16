#include "TCP-Connection.h"

#include <string.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "base/Exception.h"
#include "log/Logger.h"
#include "tcp/Buffer.h"
#include "tcp/Channel.h"
#include "tcp/EventLoop.h"

#define BUFFER_SIZE 1024  // determine how many bytes can be read into read_buffer_ at once

void TCPConnection::readNonBlocking() {
  char buf[BUFFER_SIZE];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = ::read(fd_, buf, sizeof(buf));
    if (bytes_read > 0) {  // read some bytes
      inBuffer_.append(buf, bytes_read);
    } else if (bytes_read == -1) {
      if (errno == EINTR) {  // interrupted by signal
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // done reading in non-blocking socket
        break;
      }
    } else if (bytes_read == 0) {  // peer closed connection
      LOG_TRACE << "TCPConnection::readNonBlocking peer closed connection";
      handleClose();
      return;
    } else {  // error
      LOG_ERROR << "TCPConnection::readNonBlocking error";
      handleClose();
      return;
    }
  }
}
void TCPConnection::read() { readNonBlocking(); }

int TCPConnection::writeNonBlocking() {
  ssize_t sent = ::write(fd_, outBuffer_.peek(), outBuffer_.readableBytes());
  if (sent == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {  // done writing in non-blocking socket
      return 0;
    }
    if (errno == EINTR) {  // interrupted by signal
      return 0;
    }
    handleClose();
    return -1;
  }
  // move the writerIndex_ forward
  outBuffer_.retrieve(sent);
  return 1;
}

TCPConnection::TCPConnection(EventLoop *loop, int connection_fd, int connection_id)
    : loop_(loop),
      fd_(connection_fd),
      id_(connection_id),
      lastActive_(TimeStamp::now()),
      timer_(nullptr) {
  if (nullptr == loop) {
    throw std::invalid_argument("EventLoop is nullptr.");
  }

  // create Channel
  channel_ = std::make_unique<Channel>(connection_fd, loop, true, false, true, true);  // Reading, ET by default
  /**
   *  set the Channel->read_callback_
   */
  channel_->setReadCallback([this]() {
    refreshTimeStamp();
    read();
    if (state_ == TCPState::Disconnected) {
      return;
    }
    // call on_message_callback_ if read() is successful
    if (on_message_callback_) {
      on_message_callback_(shared_from_this());
    } else {
      LOG_WARN << "on_message_callback_ callback is none";
    }
  });
  /**
   * * set the Channel->write_callback_
   */
  channel_->setWriteCallback([this]() {
    refreshTimeStamp();
    if (state_ == TCPState::Disconnected) {
      LOG_ERROR << "TCPConnection::write_callback_ called while the TCPConnection::state_ == TCPState::Disconnected";
      return;
    }
    ssize_t flag = writeNonBlocking();
    if (flag == -1) {
      LOG_ERROR << "TCPConnection::writeNonBlocking error";
    } else if (flag == 0) {
      // nothing to write
      LOG_DEBUG << "TCPConnection::writeNonBlocking write nothing";
    }
    // disable writing if there is no data left in the outBuffer_
    if (outBuffer_.readableBytes() == 0) {
      channel_->disableWriting();
    }
  });
  // notice that we will call channel_->flushEvent() in enableConnection()
}

TCPConnection::~TCPConnection() {
  ::close(fd_);
  if (contextDeleter_ && context_) {
    contextDeleter_(context_);
    context_ = nullptr;
  }
}

void TCPConnection::enableConnection() {
  state_ = TCPState::Connected;
  channel_->setTCPConnection(shared_from_this());
  if (on_connection_callback_) {
    on_connection_callback_(shared_from_this());
  }
  // use epoll_ctl(EPOLL_CTL_ADD) to activate listening ability of Channel
  channel_->flushEvent();
}

void TCPConnection::onClose(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_close_callback_ = std::move(func);
}
void TCPConnection::onConnection(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_connection_callback_ = std::move(func);
}
void TCPConnection::onMessage(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_message_callback_ = std::move(func);
}

void TCPConnection::send(const std::string &msg) { send(msg.c_str(), msg.size()); }
void TCPConnection::send(const char *msg, size_t len) {
  int left = len;
  if (state_ == TCPState::Disconnected) {
    LOG_WARN << "TCPConnection::send called while the TCPConnection::state_ == TCPState::Disconnected";
    return;
  }
  // if channel is not writing, means we have data left in the last write
  if (!channel_->isWriting() && outBuffer_.readableBytes() == 0) {
    ssize_t sent = ::write(fd_, msg, len);
    if (sent >= 0) {  // written some bytes
      left -= sent;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // the tcp write buffer is full, write nothing
        sent = 0;
      } else {  // error
        handleClose();
        return;
      }
    }
    if (left > 0) {  // still have some bytes left in the msg
      outBuffer_.append(msg + sent, left);
      // let the Channel::write_callback_ to write the rest
      channel_->enableWriting();
    }
  }
}

// EventLoop::pendingFunctors_
void TCPConnection::handleClose() {
  if (state_ == TCPState::Disconnected) {
    LOG_WARN << "TCPConnection::handleClose, handleClose called while TCPConnection::state_ == TCPState::Disconnected";
    return;
  }
  state_ = TCPState::Disconnected;
  if (on_close_callback_) {
    on_close_callback_(shared_from_this());
  } else {
    LOG_ERROR << "TCPConnection::handleClose, on_close_callback_ is nullptr";
  }
}

int TCPConnection::fd() const { return fd_; }
int TCPConnection::id() const { return id_; }
TCPState TCPConnection::connectionState() const { return state_; }
EventLoop *TCPConnection::eventLoop() const { return loop_; }
Channel *TCPConnection::channel() { return channel_.get(); }

void TCPConnection::refreshTimeStamp() { lastActive_ = TimeStamp::now(); }
TimeStamp TCPConnection::lastActive() const { return lastActive_; }
void TCPConnection::setTimer(std::shared_ptr<Timer> timer) { timer_ = std::move(timer); }
std::shared_ptr<Timer> TCPConnection::timer() const { return timer_; }
