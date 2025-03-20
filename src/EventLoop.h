#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

class Epoll;
class Channel;

class EventLoop {
private:
    Epoll *_ep;
    bool _quit;
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void loop(bool callOnce);
    void updateChannel(Channel *channel);
    // void removeChannel(Channel *channel);
};

#endif // EVENTLOOP_H_
