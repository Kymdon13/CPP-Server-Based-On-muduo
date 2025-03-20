#include <unistd.h>
#include <string.h>

#include "Channel.h"
#include "Epoll.h"
#include "util.h"

#define MAX_EVENTS 1000

Epoll::Epoll() : _epfd(-1), _evs(nullptr) { // init with false value
    _epfd = epoll_create1(0);
    errif(_epfd == -1, "epoll instance create failed");
    _evs = new epoll_event[MAX_EVENTS];
    memset(_evs, 0, sizeof(epoll_event) * MAX_EVENTS);
}

Epoll::~Epoll() {
    if (_epfd != -1) {
        close(_epfd);
    }
    if (_evs != nullptr) {
        delete[] _evs;
        _evs = nullptr;
    }
}

std::vector<Channel*> Epoll::poll(int timeout) {
    std::vector<Channel*> activeChannels;
    // nfds = -1 means error
    int nfds = epoll_wait(_epfd, _evs, MAX_EVENTS, timeout);
    errif(nfds == -1, "epoll_wait failed");
    for (int i = 0; i < nfds; ++i) {
        // ptr has been set to related channel in updateChannel
        Channel *channel = (Channel*)(_evs[i].data.ptr);
        // we just set the revents to the interested events here
        // the kernel function ep_send_events will do the matching work (to see which event is triggered)
        channel->setRevents(_evs[i].events);
        activeChannels.push_back(channel);
    }
    return activeChannels;
}

void Epoll::updateChannel(Channel *channel) {
    int fd = channel->getFd();
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = channel->getEvents();
    ev.data.ptr = channel;
    if (!channel->isInEpoll()) { // if the channel is not in epoll
        errif(epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll_ctl add event failed");
        channel->setInEpoll(true);
    } else { // if the channel is already in epoll
        errif(epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev) == -1, "epoll_ctl modify event failed");
    }
}
