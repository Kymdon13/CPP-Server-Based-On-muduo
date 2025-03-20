#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <functional>

class EventLoop;
class Socket;
class InetAddr;
class Channel;

class Connection {
private:
    EventLoop *_el;
    Socket *_sock;
    Channel *_ch_clnt;
    std::function<void(Socket*)> _delConnCallback;
public:
    Connection(EventLoop *el, Socket *sock);
    ~Connection();

    /// @brief echo message
    void echoCallback(int sockfd_clnt);

    /// @brief setter of _delConnCallback
    void setDelConnCallback(std::function<void(Socket*)> cb);
};

#endif // CONNECTION_H_
