#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>

#include "tcp/EventLoop.h"
#include "timer/TimeStamp.h"
#include "timer/Timer.h"
#include "timer/TimerQueue.h"

int main() {
  EventLoop *loop = new EventLoop();
  TimeStamp now = TimeStamp::now();
  std::cout << "Current Time: " << now.formattedString() << std::endl;

  std::cout << "Adding timer..." << std::endl;
  loop->runAfter(2.0, []() { std::cout << "Timer triggered after 2 seconds!" << std::endl; });
  loop->runAfter(3.0, [loop]() { loop->quit(); });
  std::cout << "Timer added." << std::endl;

  std::cout << "Starting event loop..." << std::endl;
  loop->loop();
  std::cout << "Program exited." << std::endl;

  delete loop;
  return 0;
}