#ifndef SERVER_H_
#define SERVER_H_

#include <unordered_map>

class Socket;
class EventLoop;
class Acceptor;
class Connection;

class Server {
private:
    EventLoop *_el;
    Acceptor *_acceptor;
    std::unordered_map<int, Connection*> conns;
public:
    Server(EventLoop *el);
    ~Server();

    void newConnCallback(Socket *sock_serv);
    void delConnCallback(Socket *sock);
};

#endif // SERVER_H_
