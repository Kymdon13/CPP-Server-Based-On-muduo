#ifndef ACCEPTOR_H_
#define ACCEPTOR_H_

#include <functional>

class EventLoop;
class Socket;
class InetAddr;
class Channel;

class Acceptor {
private:
    EventLoop *_el;
    Socket *_sock;
    InetAddr *_addr;
    Channel *_ch_acceptor;
public:
    std::function<void(Socket*)> _newConnCallback;

    Acceptor(EventLoop *el);
    ~Acceptor();

    /// @brief call _newConnCallback callback
    void acceptConn();

    /// @brief setter of _newConnCallback
    void setNewConnCallback(std::function<void(Socket*)>);
};

#endif // ACCEPTOR_H_
