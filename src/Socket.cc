#include <unistd.h>
#include <fcntl.h>

#include "Socket.h"
#include "InetAddr.h"
#include "util.h"

Socket::Socket() : _fd(-1) {
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    errif(_fd == -1, "socket create failed");
    setREUSEADDR();
}

Socket::Socket(int fd) : _fd(fd) {
    errif(_fd == -1, "socket create failed");
    setREUSEADDR();
}

Socket::~Socket() {
    if (_fd != -1) {
        close();
    }
}

void Socket::bind(InetAddr *addr) {
    errif(::bind(_fd, (struct sockaddr *)&addr->_addr, addr->_addr_len) == -1, "socket bind failed");
}

void Socket::listen(int backlog) {
    errif(::listen(_fd, backlog) == -1, "socket listen failed");
}

int Socket::accept(InetAddr *addr) {
    int sockfd_clnt = ::accept(_fd, (struct sockaddr *)&addr->_addr, &addr->_addr_len);
    errif(sockfd_clnt == -1, "socket accept failed");
    return sockfd_clnt;
}

void Socket::connect(InetAddr *addr) {
    errif(::connect(_fd, (struct sockaddr *)&addr->_addr, addr->_addr_len) == -1, "socket connect failed");
}

void Socket::setNonBlocking() {
    fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL) | O_NONBLOCK);
}

void Socket::setREUSEADDR() {
    int optval = 1; // non-zero value means using SO_REUSEADDR
    errif(-1 == setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)), "can not set SO_REUSEADDR");
}

int Socket::getFd() {
    return _fd;
}

void Socket::close() {
    errif(::close(_fd) == -1, "socket close failed");
}
