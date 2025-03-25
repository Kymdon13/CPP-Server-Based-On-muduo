#pragma once

#include <functional>
#include <memory>
#include <string>

#include "cppserver-common.h"

enum class ConnectionState : uint8_t {
  Invalid = 1,
  HandShaking,  // HandShaking = 2, ...
  Connected,
  Disconnected,
  Failed
};

class TCPConnection {
 private:
  EventLoop *loop_;

  int connection_fd_;
  int connection_id_;
  ConnectionState state_{ConnectionState::Invalid};

  // heap member belong to this object
  std::unique_ptr<Channel> channel_;
  std::unique_ptr<Buffer> read_buffer_;
  std::unique_ptr<Buffer> write_buffer_;

  std::function<void(int)> on_close_;
  std::function<void(TCPConnection *)> on_message_;

  void readNonBlocking();
  /// @brief clear the read_buffer_ and read the msg to read_buffer_
  void read();

  void writeNonBlocking();
  /// @brief send the msg in send_buffer_, and clear the send_buffer_
  void write();

 public:
  DISABLE_COPYING_AND_MOVING(TCPConnection);

  TCPConnection(EventLoop *loop, int connection_fd, int connection_id);
  ~TCPConnection();

  void SetOnCloseCallback(std::function<void(int)> const &func);
  void SetOnMessageCallback(std::function<void(TCPConnection *)> const &func);

  void SetWriteBuffer(const char *msg);

  /// @brief main interface for user
  void Send(const std::string &msg);
  void Send(const char *msg);

  void HandleMessage();
  void HandleClose();

  int GetFD() const;
  int GetID() const;
  ConnectionState GetConnectionState() const;
  EventLoop *GetEventLoop() const;
  const char *GetReadBuffer();
  const char *GetWriteBuffer();
};
