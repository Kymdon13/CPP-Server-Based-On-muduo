#include "Server.h"

#include "util.h"
#include "Socket.h"
#include "InetAddr.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"
#include "EventLoop.h"

#include <fcntl.h>
#include <iostream>
#include <functional>
#include <string.h>
#include <unistd.h>
#include <thread>

Server::Server(EventLoop *el) : _mainReactor(el), _acceptor(nullptr) {
    _acceptor = new Acceptor(_mainReactor);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnCallback, this, std::placeholders::_1);
    _acceptor->setNewConnCallback(cb);

    // create ThreadPool and add each EventLoop to the ThreadPool
    unsigned hardware_concurrency = std::thread::hardware_concurrency();
    _threadPool = new ThreadPool(hardware_concurrency);
    for (int i = 0; i < hardware_concurrency; ++i) {
        EventLoop *tmp = new EventLoop();
        _subReactor.emplace_back(tmp);
        std::function<void()> sub_loop = std::bind(&EventLoop::loop, tmp);
        _threadPool->add(sub_loop);
    }
}

Server::~Server() {
    delete _acceptor;
    delete _threadPool;
}

void Server::newConnCallback(Socket *sock_clnt) {
    if (sock_clnt->getFd() != -1) {
        // TODO dispatch sock_clnt to subReactors evenly, but we can improve it, the workload on each subReactor are not even, so we have to balance it intelligently
        int random = sock_clnt->getFd() % _subReactor.size();
        Connection *conn = new Connection(_subReactor[random], sock_clnt);
        // bind del callback
        std::function<void(Socket*)> cb = std::bind(&Server::delConnCallback, this, std::placeholders::_1);
        conn->setDelConnCallback(cb);
        // register new Connection instance into unordered_map
        conns[sock_clnt->getFd()] = conn;
    }
}

// FIXME there is a lot of problems with the dtor in the project
void Server::delConnCallback(Socket *sock) {
    int fd = sock->getFd();
    if (fd != -1) {
        if (conns.end() != conns.find(fd)) {
            // get Connection instance
            Connection *tmp = conns[fd];
            // erase fd from map
            conns.erase(fd);
            // release Connection instance
            delete tmp;
        }
    }
}
