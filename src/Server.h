#ifndef SERVER_H_
#define SERVER_H_

class Socket;
class EventLoop;
class Acceptor;

class Server {
private:
    EventLoop *_el;
    Acceptor *_acceptor;
public:
    Server(EventLoop *el);
    ~Server();

    void handleReadEvent(int sockfd_clnt);
    void handleNewConnEvent(Socket *sock_serv);
};

#endif // SERVER_H_
