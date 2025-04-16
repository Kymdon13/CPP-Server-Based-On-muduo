#pragma once

#include <functional>
#include <memory>
#include <string>

#include "Buffer.h"
#include "base/common.h"
#include "timer/TimeStamp.h"

enum class TCPState : uint8_t { Invalid = 1, Connected, Disconnected, Closing };

class EventLoop;
class Timer;
class Buffer;
class Channel;

class TCPConnection : public std::enable_shared_from_this<TCPConnection> {
 private:
  EventLoop *loop_;
  int fd_;
  int id_;
  TCPState state_{TCPState::Invalid};

  // timer
  TimeStamp lastActive_;
  std::shared_ptr<Timer> timer_;

  // heap member belong to this object
  std::unique_ptr<Channel> channel_;

  // store data that readNonBlocking read
  Buffer inBuffer_;
  // store data that TCPConnection::send hasn't sent
  Buffer outBuffer_;

  std::function<void(std::shared_ptr<TCPConnection>)> on_close_callback_;
  std::function<void(std::shared_ptr<TCPConnection>)> on_connection_callback_;
  std::function<void(std::shared_ptr<TCPConnection>)> on_message_callback_;

  // developer custom data
  void *context_{nullptr};
  std::function<void(void *)> contextDeleter_{nullptr};

  // FIXME(wzy) message boundary too simple
  /// @brief read as fast as it can, message boundary is simply when bytes_read == 0
  void readNonBlocking();
  /// @brief clear the inBuffer_ and read the msg to inBuffer_
  void read();

  /// @brief write asyncronously
  /// @return 0 if write nothing, -1 if error, >0 if write success
  int writeNonBlocking();

 public:
  DISABLE_COPYING_AND_MOVING(TCPConnection);
  TCPConnection(EventLoop *loop, int fd, int id);
  ~TCPConnection();

  /// @brief register related Channel to the system epoll, init Channel::tcpConnection_
  void enableConnection();

  void onClose(std::function<void(std::shared_ptr<TCPConnection>)> func);
  void onConnection(std::function<void(std::shared_ptr<TCPConnection>)> func);
  void onMessage(std::function<void(std::shared_ptr<TCPConnection>)> func);

  /// @brief only set the outBuffer_, you have to call private method 'write()' to send it
  void setOutBuffer(const char *msg);

  /// @brief get what is inside the inBuffer_
  Buffer *inBuffer() { return &inBuffer_; }
  Buffer *outBuffer() { return &outBuffer_; }

  /// @brief main sending interface for user
  void send(const std::string &msg);
  void send(const char *msg, size_t len);

  /// @brief call on_close_callback_
  void handleClose();

  int fd() const;
  int id() const;
  TCPState connectionState() const;
  EventLoop *eventLoop() const;
  Channel *channel();

  void refreshTimeStamp();
  TimeStamp lastActive() const;
  void setTimer(std::shared_ptr<Timer> timer);
  std::shared_ptr<Timer> timer() const;

  // context_ related
  template <typename T>
  void setContext(T *context) {
    context_ = context;
    // register deleter for a specific type
    contextDeleter_ = [](void *ptr) { delete static_cast<T *>(ptr); };
  }
  void *context() { return context_; }
};
