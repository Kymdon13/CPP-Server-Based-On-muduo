#include "Channel.h"
#include "Epoll.h"
#include "EventLoop.h"

EventLoop::EventLoop() :
    _ep(nullptr),
    _quit(false)
{
    _ep = new Epoll();
}

EventLoop::~EventLoop() {
    if (_ep != nullptr) {
        delete _ep;
        _ep = nullptr;
    }
}

void EventLoop::loop() {
    while (!_quit) {
        std::vector<Channel*> chs;
        chs = _ep->poll();
        for (auto &ch : chs) {
            ch->handleEvent();
        }
    }
}

void EventLoop::updateChannel(Channel *channel) {
    _ep->updateChannel(channel);
}
