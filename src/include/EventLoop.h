#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "cppserver-common.h"

class Poller;

class EventLoop {
 private:
  std::unique_ptr<Poller> poller_;

  std::vector<std::function<void()>> close_wait_list_;
  std::mutex close_wait_list_mutex_;
  void loop_close_wait_list_();

  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;

  pid_t tid_;

 public:
  DISABLE_COPYING_AND_MOVING(EventLoop);
  EventLoop();
  ~EventLoop();

  void Loop();
  void UpdateChannel(Channel *channel) const;
  void DeleteChannel(Channel *channel) const;

  /// @brief check if it is the sub thread calling the main_reactor's method
  bool IsMainThread();

  void CallOrQueue(std::function<void()> cb);
};
