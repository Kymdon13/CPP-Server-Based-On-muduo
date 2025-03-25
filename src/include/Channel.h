#pragma once

#include <functional>

#include "cppserver-common.h"

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
  /// @brief modify listen_event_
  void updateEvent(event_t event, bool enable);
  /// @brief call epoll_ctl
  void flushEvent();

 public:
  DISABLE_COPYING_AND_MOVING(Channel);
  Channel(int fd, EventLoop *loop) : Channel(fd, loop, true, false, true) {}
  Channel(int fd, EventLoop *loop, bool enableReading, bool enableWriting, bool useET);
  ~Channel();

  void HandleEvent() const;

  void EnableReading();
  void DisableReading();

  void EnableWriting();
  void DisableWriting();

  // TODO(wzy) there is no useET(), I integrate it into the ctor

  int GetFD() const;

  event_t GetListenEvent() const;
  event_t GetReadyEvent() const;

  bool IsInEpoll() const;
  void SetInEpoll(bool exist = true);

  void SetReadyEvents(event_t ev);

  void SetReadCallback(std::function<void()> const &callback);
  void SetWriteCallback(std::function<void()> const &callback);
};
