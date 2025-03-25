#include "include/TCP-Connection.h"

#include <string.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

#include "Buffer.h"
#include "Channel.h"
#include "Exception.h"
#include "TCP-Connection.h"

#define BUFFER_SIZE 1024  // determine how many chars can be read into read_buffer_ at once

void TCPConnection::readNonBlocking() {
  char buf[BUFFER_SIZE];
  while (true) {
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = ::read(connection_fd_, buf, sizeof(buf));
    if (bytes_read > 0) {
      read_buffer_->Append(buf, bytes_read);
    } else if (bytes_read == -1 && errno == EINTR) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // done reading in non-blocking socket
        break;
      }
      if (errno == EINTR) {  // interrupted by signal
        continue;
      }
    } else if (bytes_read == 0) {  // peer closed connection
      HandleClose();
      break;
    } else {  // error
      HandleClose();
      break;
    }
  }
}

void TCPConnection::read() {
  read_buffer_->Clear();
  readNonBlocking();
}

void TCPConnection::writeNonBlocking() {
  size_t write_buffer_size = write_buffer_->Size();
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
      break;
    }
    data_left -= bytes_write;
  }
}

void TCPConnection::write() {
  writeNonBlocking();
  write_buffer_->Clear();
}

TCPConnection::TCPConnection(EventLoop *loop, int connection_fd, int connection_id)
    : loop_(loop), connection_fd_(connection_fd), connection_id_(connection_id) {
  if (nullptr == loop) {
    throw std::invalid_argument("EventLoop is nullptr.");
  }
  channel_ = std::make_unique<Channel>(connection_fd, loop);  // Reading, ET by default
  channel_->SetReadCallback([this]() { HandleMessage(); });
}

TCPConnection::~TCPConnection() { ::close(connection_fd_); }

void TCPConnection::SetOnCloseCallback(std::function<void(int)> const &func) { on_close_ = std::move(func); }

void TCPConnection::SetOnMessageCallback(std::function<void(TCPConnection *)> const &func) {
  on_message_ = std::move(func);
}

void TCPConnection::SetWriteBuffer(const char *msg) { write_buffer_->SetBuffer(msg); }

void TCPConnection::Send(const std::string &msg) {
  SetWriteBuffer(msg.c_str());
  write();
}

void TCPConnection::Send(const char *msg) {
  SetWriteBuffer(msg);
  write();
}

void TCPConnection::HandleMessage() {
  read();
  if (on_message_) {
    on_message_(this);
  } else {
    WarnIf(true, "HandleMessage is registered while on_message_ callback is not");
  }
}

void TCPConnection::HandleClose() {
  if (state_ == ConnectionState::Disconnected) {
    WarnIf(true, "HandleClose called while the TCPConnection is closed");
    return;
  }
  state_ = ConnectionState::Disconnected;
  if (on_close_) {
    on_close_(connection_fd_);
  } else {
    WarnIf(true, "HandleClose is registered while on_close_ callback is not");
  }
}

int TCPConnection::GetFD() const { return connection_fd_; }

int TCPConnection::GetID() const { return connection_id_; }

ConnectionState TCPConnection::GetConnectionState() const { return state_; }

EventLoop *TCPConnection::GetEventLoop() const { return loop_; }

const char *TCPConnection::GetReadBuffer() { return read_buffer_->GetBuffer(); }

const char *TCPConnection::GetWriteBuffer() { return write_buffer_->GetBuffer(); }
