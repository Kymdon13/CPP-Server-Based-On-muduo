#pragma once

#include <functional>
#include <memory>
#include <string>

#include "base/cppserver-common.h"

enum class ConnectionState : uint8_t {
  Invalid = 1,
  HandShaking,  // HandShaking = 2, ...
  Connected,
  Disconnected,
  Failed
};

class TCPConnection : public std::enable_shared_from_this<TCPConnection> {
 private:
  EventLoop *loop_;

  int connection_fd_;
  int connection_id_;
  ConnectionState state_{ConnectionState::Invalid};

  // heap member belong to this object
  std::unique_ptr<Channel> channel_;
  std::unique_ptr<Buffer> read_buffer_;
  std::unique_ptr<Buffer> write_buffer_;

  /// @brief on_close_callback_ will
  std::function<void(std::shared_ptr<TCPConnection>)> on_close_callback_;
  std::function<void(std::shared_ptr<TCPConnection>)> on_connection_callback_;
  std::function<void(std::shared_ptr<TCPConnection>)> on_message_callback_;

  // FIXME(wzy) message boundary too simple
  /// @brief read as fast as it can, message boundary is simply when bytes_read == 0
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

  /// @brief register related Channel to the system epoll, init Channel::tcp_connection_ptr_
  void EnableConnection();

  void OnClose(std::function<void(std::shared_ptr<TCPConnection>)> func);
  void OnConnection(std::function<void(std::shared_ptr<TCPConnection>)> func);
  void OnMessage(std::function<void(std::shared_ptr<TCPConnection>)> func);

  /**
   * these two methods are bound together in real application
   */
  /// @brief only set the write_buffer_, you have to call private method 'write()' to send it
  void SetWriteBuffer(const char *msg);

  /// @brief get what is inside the read_buffer_
  Buffer *GetReadBuffer();

  /// @brief main sending interface for user
  void Send(const std::string &msg);
  void Send(const char *msg);

  /// @brief call on_close_callback_
  void HandleClose();

  int GetFD() const;
  int GetID() const;
  ConnectionState GetConnectionState() const;
  EventLoop *GetEventLoop() const;
  Channel *GetChannel();
  Buffer *GetWriteBuffer();
};
