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

#define BUFFER_SIZE 4096  // determine how many chars can be read into read_buffer_ at once

void TCPConnection::readNonBlocking() {
  char buf[BUFFER_SIZE];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = ::read(connection_fd_, buf, sizeof(buf));
    if (bytes_read > 0) {
      read_buffer_.append(buf, bytes_read);
    } else if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // done reading in non-blocking socket
        break;
      }
      if (errno == EINTR) {  // interrupted by signal
        continue;
      }
    } else if (bytes_read == 0) {  // peer closed connection
      HandleClose();
      return;
    } else {  // error
      LOG_ERROR << "TCPConnection::readNonBlocking error";
      HandleClose();
      return;
    }
  }
}
void TCPConnection::read() {
  read_buffer_->Clear();
  return readNonBlocking();
}

ssize_t TCPConnection::writeNonBlocking() {
  int left = write_buffer_.readableBytes();
  ssize_t sent = ::write(connection_fd_, write_buffer_.peek(), left);
  if (sent == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {  // done writing in non-blocking socket
      return 0;
    }
    if (errno == EINTR) {  // interrupted by signal
      return 0;
    }
    // FIXME(wzy) there are other situations we have not list here
    HandleClose();
    return -1;
  }
  // move the writerIndex_ forward
  write_buffer_.retrieve(sent);
  return sent;
}

TCPConnection::TCPConnection(EventLoop *loop, int connection_fd, int connection_id)
    : loop_(loop),
      connection_fd_(connection_fd),
      connection_id_(connection_id),
      last_active_time_(TimeStamp::Now()),
      timer_(nullptr)
{
  if (nullptr == loop) {
    throw std::invalid_argument("EventLoop is nullptr.");
  }

  // create Channel
  channel_ = std::make_unique<Channel>(connection_fd, loop, true, false, true, true);  // Reading, ET by default
  /**
   *  set the Channel->read_callback_
   */
  channel_->SetReadCallback([this]() {
    RefreshTimeStamp();
    read();
    if (state_ == TCPState::Disconnected) {
      return;
    }
    // call on_message_callback_ if read() is successful
    if (on_message_callback_) {
      on_message_callback_(shared_from_this());
    } else {
      WarnIf(true, "on_message_callback_ callback is none");
    }
  });
  /**
   * * set the Channel->write_callback_
   */
  channel_->SetWriteCallback([this]() {
    RefreshTimeStamp();
    if (state_ == TCPState::Disconnected) {
      LOG_ERROR << "TCPConnection::write_callback_ called while the TCPConnection::state_ == TCPState::Disconnected";
      return;
    }
    ssize_t sent = writeNonBlocking();
    if (sent == -1) {
      LOG_ERROR << "TCPConnection::writeNonBlocking error";
    } else if (sent == 0) {
      // nothing to write
      LOG_DEBUG << "TCPConnection::writeNonBlocking write nothing";
    }
  });
  // notice that we will call channel_->FlushEvent() in EnableConnection()
}

TCPConnection::~TCPConnection() { ::close(connection_fd_); }

void TCPConnection::EnableConnection() {
  state_ = TCPState::Connected;
  channel_->SetTCPConnectionPtr(shared_from_this());
  // use epoll_ctl(EPOLL_CTL_ADD) to really start listening on events
  channel_->FlushEvent();
  if (on_connection_callback_) {
    on_connection_callback_(shared_from_this());
  }
}

void TCPConnection::OnClose(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_close_callback_ = std::move(func);
}
void TCPConnection::OnConnection(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_connection_callback_ = std::move(func);
}
void TCPConnection::OnMessage(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_message_callback_ = std::move(func);
}

void TCPConnection::Send(const std::string &msg) {
  Send(msg.c_str(), msg.size());
}
void TCPConnection::Send(const char *msg, size_t len) {
  int left = len;
  if (state_ == TCPState::Disconnected) {
    LOG_WARN << "TCPConnection::Send called while the TCPConnection::state_ == TCPState::Disconnected";
    return;
  }
  if (!channel_->isWriting() && write_buffer_.readableBytes() == 0) {
    ssize_t sent = ::write(connection_fd_, msg, len);
    if (sent >= 0) {  // write some bytes
      left -= sent;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // write buffer is full, write nothing
        sent = 0;
      } else {  // error
        HandleClose();
        return;
      }
    }
    if (left > 0) {  // still have some bytes left in the msg
      write_buffer_.append(msg + sent, left);
      // let the Channel::write_callback_ to write the rest
      channel_->enableWriting();
    }
  }
}

// EventLoop::pendingFunctors_
void TCPConnection::HandleClose() {
  if (state_ == TCPState::Disconnected) {
    WarnIf(true, "HandleClose called while the TCPConnection::state_ == TCPState::Disconnected");
    return;
  }
  state_ = TCPState::Disconnected;
  if (on_close_callback_) {
    on_close_callback_(shared_from_this());
  } else {
    WarnIf(true, "HandleClose is registered while on_close_callback_ callback is not");
  }
}

int TCPConnection::GetFD() const { return connection_fd_; }
int TCPConnection::GetID() const { return connection_id_; }
TCPState TCPConnection::GetConnectionState() const { return state_; }
EventLoop *TCPConnection::GetEventLoop() const { return loop_; }
Channel *TCPConnection::GetChannel() { return channel_.get(); }

void TCPConnection::RefreshTimeStamp() { last_active_time_ = TimeStamp::Now(); }
TimeStamp TCPConnection::GetLastActiveTime() const { return last_active_time_; }
void TCPConnection::SetTimer(std::shared_ptr<Timer> timer) { timer_ = std::move(timer); }
std::shared_ptr<Timer> TCPConnection::GetTimer() const { return timer_; }
