#ifndef SERVER_H_
#define SERVER_H_

#include <vector>
#include <unordered_map>

class Socket;
class EventLoop;
class Acceptor;
class Connection;
class ThreadPool;

class Server {
private:
    EventLoop *_mainReactor;
    std::vector<EventLoop*> _subReactor;
    Acceptor *_acceptor;
    std::unordered_map<int, Connection*> conns;
    ThreadPool *_threadPool;
public:
    Server(EventLoop *el);
    ~Server();

    void newConnCallback(Socket *sock_serv);
    void delConnCallback(Socket *sock);
};

#endif // SERVER_H_
