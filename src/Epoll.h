#ifndef EPOLL_H_
#define EPOLL_H_

#include <sys/epoll.h>
#include <vector>

class Channel;

class Epoll {
private:
    int _epfd;
    struct epoll_event *_evs;
public:
    Epoll();
    ~Epoll();

    void addFd(int fd, uint32_t op);
    std::vector<Channel*> poll(int timeout = -1);

    /**
     * * @brief update the channel in epoll
     * * @param channel the channel to update
     */
    void updateChannel(Channel *channel);
};

#endif // EPOLL_H_
