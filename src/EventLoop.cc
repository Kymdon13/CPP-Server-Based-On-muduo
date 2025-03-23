#include "Channel.h"
#include "Epoll.h"
#include "EventLoop.h"
#include "ThreadPool.h"

EventLoop::EventLoop() :
    _ep(nullptr),
    _quit(false),
    _threadPool(nullptr)
{
    _ep = new Epoll();
    _threadPool = new ThreadPool();
}

EventLoop::~EventLoop() {
    if (_ep != nullptr) {
        delete _ep;
        _ep = nullptr;
    }
    if (_threadPool != nullptr) {
        delete _threadPool;
        _threadPool = nullptr;
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

void EventLoop::loopOnce() {
    std::vector<Channel*> chs;
    chs = _ep->poll();
    for (auto &ch : chs) {
        ch->handleEvent();
    }
}

void EventLoop::updateChannel(Channel *channel) {
    _ep->updateChannel(channel);
}

void EventLoop::addThread(std::function<void()> func) {
    _threadPool->add(func);
}
