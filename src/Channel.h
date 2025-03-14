#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <sys/epoll.h>
#include <functional>

class EventLoop;

class Channel {
private:
    EventLoop *_el;
    int _fd;
    uint32_t _events;
    uint32_t _revents;
    bool _inEpoll;
    std::function<void()> _callback;
public:
    Channel(EventLoop *el, int fd);
    ~Channel();

    /// @brief set EPOLLIN | EPOLLET flag for _fd register in _el._ep
    void enableReading();

    /// @brief getter of _fd
    /// @return _fd
    int getFd() const;

    /// @brief getter of _inEpoll
    /// @return _inEpoll
    bool isInEpoll() const;

    /// @brief set the _inEpoll flag
    /// @param inEpoll pass false if you want to disable this Channel
    void setInEpoll(bool inEpoll = true);

    /// @brief getter of _events, you can use it to check the original event type you registerd
    /// @return _events
    uint32_t getEvents() const;

    /// @brief setter of _events
    /// @param events event you want this Channel to monitor
    void setEvents(uint32_t events);

    /// @brief getter of _revents
    /// @return _revents
    uint32_t getRevents() const;

    /// @brief setter of _revents, do not use this directly
    /// @param revents used by epoll instance
    void setRevents(uint32_t revents);

    /// @brief call the _callback()
    void handleEvent();

    /// @brief register callback function
    /// @param callback used to set _callback
    void setCallback(std::function<void()> callback);
};

#endif // CHANNEL_H_
