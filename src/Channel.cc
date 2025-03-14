#include "Epoll.h"
#include "EventLoop.h"
#include "Channel.h"

Channel::Channel(EventLoop *el, int fd) :
    _el(el),
    _fd(fd),
    _events(0),
    _revents(0),
    _inEpoll(false) {}

Channel::~Channel() {}

void Channel::enableReading() {
    _events = EPOLLIN | EPOLLET;
    _el->updateChannel(this);
}

int Channel::getFd() const { return _fd; }

bool Channel::isInEpoll() const { return _inEpoll; }

void Channel::setInEpoll(bool inEpoll) { _inEpoll = inEpoll; }

uint32_t Channel::getEvents() const { return _events; }

void Channel::setEvents(uint32_t events) { _events = events; }

uint32_t Channel::getRevents() const { return _revents; }

void Channel::setRevents(uint32_t revents) { _revents = revents; }

void Channel::handleEvent() { _callback(); }

void Channel::setCallback(std::function<void()> callback) { _callback = callback; }
