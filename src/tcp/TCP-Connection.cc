#include "TCP-Connection.h"

#include <fcntl.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
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

TCPConnection::TCPConnection(EventLoop* loop, int connection_fd, int connection_id)
    : loop_(loop), fd_(connection_fd), id_(connection_id), lastActive_(TimeStamp::now()), timer_(nullptr) {
  if (nullptr == loop) {
    throw std::invalid_argument("EventLoop is nullptr.");
  }

  // create Channel
  channel_ = std::make_unique<Channel>(connection_fd, loop, true, false, true, true);  // Reading, ET by default
  /**
   *  set the Channel->read_callback_, EPOLLIN will trigger when the read buffer of the socket turns from empty to not
   * empty
   */
  channel_->setReadCallback([this]() {
    refreshTimeStamp();
    readNonBlocking();
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
   * * set the Channel->write_callback_, EPOLLOUT will trigger when the write buffer of the socket turns from full to
   * not full
   */
  channel_->setWriteCallback([this]() {
    refreshTimeStamp();
    if (state_ == TCPState::Disconnected) {
      LOG_ERROR << "TCPConnection::write_callback_ called while the TCPConnection::state_ == TCPState::Disconnected";
      return;
    }
    /* write outBuffer_ */
    ssize_t flag = writeNonBlocking();
    if (flag == -1) {
      LOG_ERROR << "TCPConnection::writeNonBlocking error";
    } else if (flag == 0) {
      // nothing to write
      LOG_DEBUG << "TCPConnection::writeNonBlocking write nothing";
    }
    /* write file using sendfile() */
    /* note that it is possible the sendfile does not fill the write buffer at onece,
    if first FileInfo has only a few bytes to send, after we send it we can pop it out
    and move on to the next FileInfo */
    while (!file_list_.empty()) {
      auto& file_info = file_list_.front();
      off_t sent_once = ::sendfile(fd_, file_info.fd__, &file_info.sent__, file_info.size__ - file_info.sent__);
      if (sent_once < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {  // the tcp write buffer is full, write nothing
          break;
        } else {
          LOG_ERROR << "TCPConnection::write_callback_ sendfile error";
          handleClose();
          return;
        }
      }
      if (file_info.sent__ >= file_info.size__) {
        ::close(file_info.fd__);
        file_list_.pop_front();
      }
    }
    // disable writing if done writing the outBuffer_ and sending the file_list_
    if (outBuffer_.readableBytes() == 0 && file_list_.empty()) {
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

void TCPConnection::send(const std::string& msg) { send(msg.c_str(), msg.size()); }
void TCPConnection::send(const char* msg, size_t len) {
  ssize_t sent = 0;
  size_t left = len;
  // if isWriting() returns true, means we have data left in the last write,
  if (!channel_->isWriting() && outBuffer_.readableBytes() == 0) {
    sent = ::write(fd_, msg, len);
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
  }
  // if we have bytes left in the msg, put it into outBuffer_
  if (left > 0) {
    outBuffer_.append(msg + sent, left);
    // let the Channel::write_callback_ to write the rest
    if (!channel_->isWriting()) channel_->enableWriting();
  }
}
void TCPConnection::sendFile(int file_fd) {
  struct stat st;
  ::fstat(file_fd, &st);

  off_t sent_total = 0;
  if (!channel_->isWriting() && file_list_.empty()) {
    ssize_t sent_once = ::sendfile(fd_, file_fd, &sent_total, st.st_size - sent_total);
    if (sent_once < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // the tcp write buffer is full, write nothing
        sent_once = 0;
      } else {
        LOG_ERROR << "TCPConnection::sendFile, sendfile error";
        handleClose();
        return;
      }
    }
  }
  // if we haven't sent them all, put the fd and sent_total into the file_list_
  if (sent_total < st.st_size) {
    file_list_.emplace_back(file_fd, st.st_size, sent_total);
    channel_->enableWriting();
  } else {
    ::close(file_fd);
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

void TCPConnection::onClose(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_close_callback_ = std::move(func);
}
void TCPConnection::onConnection(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_connection_callback_ = std::move(func);
}
void TCPConnection::onMessage(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_message_callback_ = std::move(func);
}
