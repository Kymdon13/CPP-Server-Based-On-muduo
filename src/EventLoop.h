#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include <functional>

class Epoll;
class Channel;
class ThreadPool;

class EventLoop {
private:
    Epoll *_ep;
    bool _quit;
    ThreadPool *_threadPool;
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void loopOnce();
    void updateChannel(Channel *channel);
    // void removeChannel(Channel *channel);
    void addThread(std::function<void()> func);
};

#endif // EVENTLOOP_H_
