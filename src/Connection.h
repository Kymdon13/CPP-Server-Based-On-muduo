#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <functional>

typedef unsigned __flag_t;

#define FLAGECHO ((__flag_t)(1U))
#define FLAGPRINT ((__flag_t)(1U << 1))

class EventLoop;
class Socket;
class InetAddr;
class Channel;
class Buffer;

class Connection {
private:
    EventLoop *_el;
    Socket *_sock;
    Channel *_ch;
    std::string *_inBuffer;
    Buffer *_readBuffer;
    std::function<void(Socket*)> _delConnCallback;
public:
    Connection(EventLoop *el, Socket *sock, __flag_t flag = FLAGECHO);
    ~Connection();

    void printCallback(Socket *sock);

    /// @brief echo message
    void echoCallback(int sockfd_clnt);

    /// @brief setter of _delConnCallback
    void setDelConnCallback(std::function<void(Socket*)> cb);
};

#endif // CONNECTION_H_
