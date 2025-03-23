#include "Acceptor.h"
#include "Socket.h"
#include "InetAddr.h"
#include "Channel.h"
#include "Server.h"

#include <iostream>

Acceptor::Acceptor(EventLoop *el) :
    _el(el),
    _sock(nullptr),
    _ch_acceptor(nullptr)
{
    // create a socket, we want a blocking socket now cause accept a connection ain't gonna take too long
    _sock = new Socket();
    // create an address
    InetAddr *_addr = new InetAddr("127.0.0.1", 8888);
    // bind the socket to the address
    _sock->bind(_addr);
    // listen for incoming connections
    _sock->listen();

    _ch_acceptor = new Channel(_el, _sock->getFd());
    // set the callback function for the channel
    std::function<void()> cb = std::bind(&Acceptor::acceptConn, this); // in the calling process of acceptConn we can directly access _sock
    _ch_acceptor->setReadCallback(cb);
    // enable reading on the channel, and not enable EPOLLET cause we want epoll_wait to return the connection events all the time if they are not handled
    _ch_acceptor->enableReading();
    // no need to use ThreadPool for such a simple task
    // _ch_acceptor->setUseThreadPool(false);

    delete _addr;
}

Acceptor::~Acceptor() {
    delete _sock;
    delete _ch_acceptor;
}

void Acceptor::acceptConn() {
    InetAddr *addr_clnt = new InetAddr();

    Socket *sock_clnt = new Socket(_sock->accept(addr_clnt));
    sock_clnt->setNonBlocking();

    std::cout << "new client fd: " << sock_clnt->getFd() <<
        " IP: " << inet_ntoa(addr_clnt->_addr.sin_addr) <<
        " Port: " << ntohs(addr_clnt->_addr.sin_port) << std::endl;

    // call the callback
    _newConnCallback(sock_clnt);

    // release the temp addr_clnt
    delete addr_clnt;
}

void Acceptor::setNewConnCallback(std::function<void(Socket*)> cb) {
    _newConnCallback = cb;
}
