#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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

size_t Socket::read(void *buf, size_t n, bool &done) {
    ssize_t read_bytes = ::read(_fd, buf, n);
    if (read_bytes == -1) { // error
        if (errno == EAGAIN) {
            done = true;
            return 0; // done reading, return safely
        } else {
            close();
            errif(true, "socket read failed");
        }
    } else if (read_bytes == 0) { // peer has closed the socket
        std::cerr << "peer with fd: " << _fd << " has closed socket, closing this socket..." << std::endl;
        close();
    }
    return strlen((char*)buf); // FIXME may cause problem if buf contains no bytes or isn't end with '\0'
}

size_t Socket::readSync(void *buf, size_t n) {
    ssize_t read_bytes = ::read(_fd, buf, n);
    if (read_bytes == -1) { // error
        close();
        errif(true, "socket read failed");
    } else if (read_bytes == 0) { // peer has closed the socket
        std::cerr << "peer with fd: " << _fd << " has closed socket, closing this socket..." << std::endl;
        close();
        return 0;
    }
    return strlen((char*)buf); // FIXME may cause problem if buf contains no bytes or isn't end with '\0'
}

size_t Socket::writeSync(const void *buf, size_t nbytes) {
    ssize_t write_bytes = ::write(_fd, buf, nbytes);
    if (write_bytes == -1) { // error
        close();
        errif(true, "socket send failed");
    } else if (write_bytes == 0) // no data sent
        std::cerr << "no data sent" << std::endl;
    return strlen((char*)buf);
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

bool Socket::getIsClosed() {
    return _isClosed;
}

void Socket::close() {
    if (!_isClosed && _fd != -1) {
        errif(::close(_fd) == -1, "socket close failed");
        _isClosed = true;
    }
}
