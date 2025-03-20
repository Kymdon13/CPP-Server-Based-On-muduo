#include "Server.h"

#include "util.h"
#include "Socket.h"
#include "InetAddr.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Connection.h"

#include <fcntl.h>
#include <iostream>
#include <functional>
#include <string.h>
#include <unistd.h>

Server::Server(EventLoop *el) : _el(el), _acceptor(nullptr) {
    _acceptor = new Acceptor(_el);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnCallback, this, std::placeholders::_1);
    _acceptor->setNewConnCallback(cb);
}

Server::~Server() {
    delete _acceptor;
}

void Server::newConnCallback(Socket *sock_clnt) {
    Connection *conn = new Connection(_el, sock_clnt);
    // bind del callback
    std::function<void(Socket*)> cb = std::bind(&Server::delConnCallback, this, std::placeholders::_1);
    conn->setDelConnCallback(cb);
    // register new Connection instance into unordered_map
    conns[sock_clnt->getFd()] = conn;
}

void Server::delConnCallback(Socket *sock_clnt) {
    // get Connection instance
    Connection *tmp = conns[sock_clnt->getFd()];
    // erase fd from map
    conns.erase(sock_clnt->getFd());
    // release Connection instance
    delete tmp;
}
