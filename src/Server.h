#ifndef SERVER_H_
#define SERVER_H_

class Socket;
class EventLoop;

class Server {
private:
    EventLoop *_loop;
public:
    Server(EventLoop *loop);
    ~Server();

    void handleReadEvent(int sockfd_clnt);
    void handleNewConnEvent(Socket *sock_serv);
};

#endif // SERVER_H_
