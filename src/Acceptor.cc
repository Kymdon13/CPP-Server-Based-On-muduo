#include "Acceptor.h"
#include "Socket.h"
#include "InetAddr.h"
#include "Channel.h"
#include "Server.h"

Acceptor::Acceptor(EventLoop *el) : _el(el) {
    // create a socket
    _sock = new Socket();
    // create an address
    _addr = new InetAddr("127.0.0.1", 8888);
    // set the socket to non-blocking mode
    _sock->setNonBlocking();
    // bind the socket to the address
    _sock->bind(_addr);
    // listen for incoming connections
    _sock->listen();

    _ch_acceptor = new Channel(_el, _sock->getFd());
    // set the callback function for the channel
    std::function<void()> cb = std::bind(&Acceptor::acceptConn, this); // in the calling process of acceptConn we can directly access _sock
    _ch_acceptor->setCallback(cb);
    // enable reading on the channel
    _ch_acceptor->enableReading();
}

Acceptor::~Acceptor() {
    delete _sock;
    delete _addr;
    delete _ch_acceptor;
}

void Acceptor::acceptConn() {
    _newConnCallback(_sock);
}

void Acceptor::setNewConnCallback(std::function<void(Socket*)> cb) {
    _newConnCallback = cb;
}
