#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>

#include "timer/TimeStamp.h"
#include "timer/Timer.h"
#include "timer/TimerQueue.h"
#include "tcp/EventLoop.h"

int main() {
    EventLoop* loop = new EventLoop();
    TimeStamp now = TimeStamp::Now();
    std::cout << "Current Time: " << now.ToFormattedString() << std::endl;

    std::cout << "Adding timer..." << std::endl;
    loop->RunAfter(2.0, []() {
        std::cout << "Timer triggered after 2 seconds!" << std::endl;
    });
    loop->RunAfter(3.0, [loop]() {
        loop->Quit();
    });
    std::cout << "Timer added." << std::endl;

    std::cout << "Starting event loop..." << std::endl;
    loop->Loop();
    std::cout << "Program exited." << std::endl;

    delete loop;
    return 0;
}