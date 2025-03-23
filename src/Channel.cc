#include "Epoll.h"
#include "EventLoop.h"
#include "Channel.h"

#include <unistd.h>
#include <sys/epoll.h>

Channel::Channel(EventLoop *el, int fd) :
    _el(el),
    _fd(fd),
    _events(0),
    _revents(0),
    _inEpoll(false) {}

Channel::~Channel() {
    if (_fd != -1) {
        close(_fd);
        _fd = -1;
    }
}

// TODO if the setting of _events not going to change for the rest of the life of Channel, why not put it in ctor
void Channel::enableReading() {
    _events |= EPOLLIN | EPOLLPRI;
    _el->updateChannel(this);
}

// TODO if the setting of _events not going to change for the rest of the life of Channel, why not put it in ctor
void Channel::enableEPOLLET() {
    _events |= EPOLLET;
    _el->updateChannel(this);
}

int Channel::getFd() const { return _fd; }

bool Channel::isInEpoll() const { return _inEpoll; }

void Channel::setInEpoll(bool inEpoll) { _inEpoll = inEpoll; }

uint32_t Channel::getEvents() const { return _events; }

uint32_t Channel::getRevents() const { return _revents; }

void Channel::setRevents(uint32_t revents) { _revents = revents; }

void Channel::handleEvent() {
    if (_revents & (EPOLLIN | EPOLLPRI)) { // EPOLLPRI means higher priority data arriving
        _readCallback();
    }
    if (_revents & (EPOLLOUT)) {
        _writeCallback();
    }
}

void Channel::setReadCallback(std::function<void()> callback) { _readCallback = callback; }

void Channel::setWriteCallback(std::function<void()> callback) { _writeCallback = callback; }
