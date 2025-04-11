#pragma once

#include <functional>
#include <memory>
#include <string>

#include "Buffer.h"
#include "base/cppserver-common.h"
#include "timer/TimeStamp.h"

enum class TCPState : uint8_t { Invalid = 1, Connected, Disconnected, Closing };

class EventLoop;
class Timer;
class Buffer;
class Channel;

class TCPConnection : public std::enable_shared_from_this<TCPConnection> {
 private:
  EventLoop *loop_;
  int connection_fd_;
  int connection_id_;
  TCPState state_{TCPState::Invalid};
  TimeStamp last_active_time_;
  std::shared_ptr<Timer> timer_;

  // heap member belong to this object
  std::unique_ptr<Channel> channel_;

  Buffer read_buffer_;
  Buffer write_buffer_;

  std::function<void(std::shared_ptr<TCPConnection>)> on_close_callback_;
  std::function<void(std::shared_ptr<TCPConnection>)> on_connection_callback_;
  std::function<void(std::shared_ptr<TCPConnection>)> on_message_callback_;

  // FIXME(wzy) message boundary too simple
  /// @brief read as fast as it can, message boundary is simply when bytes_read == 0
  void readNonBlocking();
  /// @brief clear the read_buffer_ and read the msg to read_buffer_
  void read();

  /// @brief write asyncronously
  /// @return 0 if write nothing, -1 if error, >0 if write success
  ssize_t writeNonBlocking();

 public:
  DISABLE_COPYING_AND_MOVING(TCPConnection);
  TCPConnection(EventLoop *loop, int connection_fd, int connection_id);
  ~TCPConnection();

  /// @brief register related Channel to the system epoll, init Channel::tcp_connection_ptr_
  void EnableConnection();

  void OnClose(std::function<void(std::shared_ptr<TCPConnection>)> func);
  void OnConnection(std::function<void(std::shared_ptr<TCPConnection>)> func);
  void OnMessage(std::function<void(std::shared_ptr<TCPConnection>)> func);

  /// @brief only set the write_buffer_, you have to call private method 'write()' to send it
  void SetWriteBuffer(const char *msg);

  /// @brief get what is inside the read_buffer_
  Buffer *readBuffer() { return &read_buffer_; }
  Buffer *writeBuffer() { return &write_buffer_; }

  /// @brief main sending interface for user
  void Send(const std::string &msg);
  void Send(const char *msg, size_t len);

  /// @brief call on_close_callback_
  void HandleClose();

  int GetFD() const;
  int GetID() const;
  TCPState GetConnectionState() const;
  EventLoop *GetEventLoop() const;
  Channel *GetChannel();

  void RefreshTimeStamp();
  TimeStamp GetLastActiveTime() const;
  void SetTimer(std::shared_ptr<Timer> timer);
  std::shared_ptr<Timer> GetTimer() const;
};
