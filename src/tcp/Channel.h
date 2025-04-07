#pragma once

#include <functional>
#include <memory>

#include "base/cppserver-common.h"

class Channel {
 private:
  int fd_;
  EventLoop *loop_;
  event_t listen_event_;
  event_t ready_event_;
  bool in_epoll_{false};
  std::function<void()> read_callback_;
  std::function<void()> write_callback_;

  bool flushable_{false};

  std::weak_ptr<TCPConnection> tcp_connection_ptr_;

  /// @brief modify listen_event_
  void updateEvent(event_t event, bool enable);

 public:
  DISABLE_COPYING_AND_MOVING(Channel);
  Channel(int fd, EventLoop *loop) : Channel(fd, loop, true, false, true) {}
  Channel(int fd, EventLoop *loop, bool enableReading, bool enableWriting, bool useET);
  ~Channel();

  /// @brief call epoll_ctl
  void FlushEvent();

  void HandleEvent() const;

  void EnableReading();
  void DisableReading();

  void EnableWriting();
  void DisableWriting();

  void DisableAll();

  // XXX(wzy) there is no useET(), it is integrated into the ctor

  int GetFD() const;

  void Remove();

  event_t GetListenEvent() const;
  event_t GetReadyEvent() const;

  bool IsInEpoll() const;
  void SetInEpoll(bool exist = true);

  void SetReadyEvents(event_t ev);

  void SetReadCallback(const std::function<void()> &callback);
  void SetWriteCallback(const std::function<void()> &callback);

  void SetTCPConnectionPtr(std::shared_ptr<TCPConnection> conn);
};
