#include "TCP-Connection.h"

#include <string.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "base/Exception.h"
#include "tcp/Buffer.h"
#include "tcp/Channel.h"
#include "tcp/EventLoop.h"

#define BUFFER_SIZE 4096  // determine how many chars can be read into read_buffer_ at once

bool TCPConnection::readNonBlocking() {
  char buf[BUFFER_SIZE];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = ::read(connection_fd_, buf, sizeof(buf));
    if (bytes_read > 0) {
      read_buffer_->Append(buf, bytes_read);
    } else if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // done reading in non-blocking socket
        break;
      }
      if (errno == EINTR) {  // interrupted by signal
        continue;
      }
    } else if (bytes_read == 0) {  // peer closed connection
      HandleClose();
      return false;
    } else {  // error
      HandleClose();
      return false;
    }
  }
  return true;
}
bool TCPConnection::read() {
  read_buffer_->Clear();
  return readNonBlocking();
}

bool TCPConnection::writeNonBlocking() {
  size_t write_buffer_size = write_buffer_->Size();
  // create a temp buf of type 'char[]', cuz ::write() does not support string
  char buf[write_buffer_size];
  memcpy(buf, write_buffer_->GetBuffer(), write_buffer_size);
  int data_size = write_buffer_size;
  int data_left = write_buffer_size;
  while (data_left > 0) {
    ssize_t bytes_write = ::write(connection_fd_, buf + data_size - data_left, data_left);
    if (bytes_write == -1) {
      if (errno == EAGAIN) {
        break;
      }
      if (errno == EINTR) {
        continue;
      }
      // other situation is error
      HandleClose();
      return false;
    }
    data_left -= bytes_write;
  }
  return true;
}
bool TCPConnection::write() {
  bool closing = writeNonBlocking();
  write_buffer_->Clear();
  return closing;
}

TCPConnection::TCPConnection(EventLoop *loop, int connection_fd, int connection_id)
    : loop_(loop),
      connection_fd_(connection_fd),
      connection_id_(connection_id),
      last_active_time_(TimeStamp::Now()),
      timer_(nullptr) {
  if (nullptr == loop) {
    throw std::invalid_argument("EventLoop is nullptr.");
  }

  // create Channel
  channel_ = std::make_unique<Channel>(connection_fd, loop, true, false, true, true);  // Reading, ET by default
  // set the Channel->read_callback_
  channel_->SetReadCallback([this]() {
    RefreshTimeStamp();
    if (!read()) {
      return;
    }
    // call on_message_callback_ if read() is successful
    if (on_message_callback_) {
      on_message_callback_(shared_from_this());
    } else {
      WarnIf(true, "on_message_callback_ callback is none");
    }
  });
  // notice that we will call channel_->FlushEvent() in EnableConnection()

  // create read buffer and write buffer
  read_buffer_ = std::make_unique<Buffer>();
  write_buffer_ = std::make_unique<Buffer>();
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

void TCPConnection::SetWriteBuffer(const char *msg) { write_buffer_->SetBuffer(msg); }
Buffer *TCPConnection::GetReadBuffer() { return read_buffer_.get(); }

bool TCPConnection::Send(const std::string &msg) {
  SetWriteBuffer(msg.c_str());
  return write();
}
bool TCPConnection::Send(const char *msg) {
  SetWriteBuffer(msg);
  return write();
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
Buffer *TCPConnection::GetWriteBuffer() { return write_buffer_.get(); }

void TCPConnection::RefreshTimeStamp() { last_active_time_ = TimeStamp::Now(); }
TimeStamp TCPConnection::GetLastActiveTime() const { return last_active_time_; }
void TCPConnection::SetTimer(std::shared_ptr<Timer> timer) { timer_ = std::move(timer); }
std::shared_ptr<Timer> TCPConnection::GetTimer() const { return timer_; }
