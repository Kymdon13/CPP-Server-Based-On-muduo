#pragma once

#include <sys/epoll.h>

#include <functional>
#include <memory>

#include "base/common.h"

class EventLoop;
class TCPConnection;

class Channel {
 private:
  int fd_;
  EventLoop* loop_;
  event_t listenEvent_;
  event_t readyEvent_;
  bool in_epoll_{false};
  std::function<void()> read_callback_;
  std::function<void()> write_callback_;

  bool flushable_{false};

  std::weak_ptr<TCPConnection> tcpConnection_;

  bool isConnection_{false};

  /// @brief modify listenEvent_
  void updateEvent(event_t event, bool enable);

 public:
  DISABLE_COPYING_AND_MOVING(Channel);
  /// @brief Channel constructor
  /// @param fd fd to monitor
  /// @param loop EventLoop to which this Channel belongs
  /// @param enableReading listen for read events
  /// @param enableWriting listen for write events
  /// @param useET use edge-triggered mode
  /// @param is_connection is this a channel belongs to a TCPConnection
  Channel(int fd, EventLoop* loop, bool enableReading, bool enableWriting, bool useET, bool is_connection);
  ~Channel();

  /// @brief after creating a Channel or update the listening event, use this to call the epoll_ctl to actually register
  /// it into epoll
  void flushEvent();

  void handleEvent() const;

  void enableReading();
  void disableReading();
  bool isReading() const { return listenEvent_ & (EPOLLIN | EPOLLPRI); }

  void enableWriting();
  void disableWriting();
  bool isWriting() const { return listenEvent_ & EPOLLOUT; }

  void disableAll();

  // XXX(wzy) there is no useET(), it is integrated into the ctor

  int fd() const;

  void removeSelf();

  event_t listenEvent() const;
  event_t readyEvent() const;

  bool isInEpoll() const;
  void setInEpoll(bool exist = true);

  void setReadyEvents(event_t ev);

  void setReadCallback(std::function<void()> callback);
  void setWriteCallback(std::function<void()> callback);

  void setTCPConnection(std::shared_ptr<TCPConnection> conn);
};
