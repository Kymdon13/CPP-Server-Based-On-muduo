#include "Connection.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "util.h"

#include <unistd.h>
#include <string.h>
#include <iostream>

#define BUFFER_SIZE 1024

Connection::Connection(EventLoop *el, Socket *sock, __flag_t flag) :
    _el(el),
    _sock(sock),
    _ch(nullptr),
    _inBuffer(new std::string()),
    _readBuffer(nullptr)
{
    _ch = new Channel(_el, _sock->getFd());
    // bind callback to _ch according to flag
    std::function<void()> cb;
    if (flag & FLAGECHO) {
        cb = std::bind(&Connection::echoCallback, this, _sock->getFd());
    } else if (flag & FLAGPRINT) {
        cb = std::bind(&Connection::printCallback, this, _sock);
    }
    _ch->setCallback(cb);
    _ch->enableReading();
    _readBuffer = new Buffer();
}

Connection::~Connection() {
    delete _sock;
    delete _ch;
    delete _inBuffer;
    delete _readBuffer;
}

void Connection::printCallback(Socket *sock) {
    char buffer[BUFFER_SIZE]; // buffer to store received data
    memset(buffer, 0, sizeof(buffer));
    size_t bytes_read = sock->readSync(buffer, sizeof(buffer));
    std::cerr << bytes_read << " bytes received from server:" << buffer << std::endl;
}

void Connection::echoCallback(int sockfd_clnt) {
    char buffer[BUFFER_SIZE]; // buffer to store received data
    ssize_t bytes_read;
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // clear the buffer
        // receives data from the socket and stores it in the _readBuffer
        // then handle the received data
        bytes_read = read(sockfd_clnt, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            _readBuffer->append(buffer, bytes_read); // store it in the _readBuffer temporarily
        } else if (bytes_read == -1) {
            if (errno == EAGAIN) {
                // has read all the data into the _readBuffer
                std::cout << _readBuffer->size() << " bytes received from client:" << buffer << std::endl;
                std::cout << "client " << sockfd_clnt << " done reading" << std::endl;
                // until we reach the end of the message, write data to client
                errif(-1 == write(sockfd_clnt, _readBuffer->c_str(), _readBuffer->size()), "socket write error");
                _readBuffer->clear();
                break;
            } else if (errno == EINTR) {
                std::cout << "interrupt by signal, continue reading..." << std::endl;
                continue;
            } else {
                // error
                close(sockfd_clnt);
                break;
            }
        } else if (bytes_read == 0) { // client has closed the socket
            std::cout << "client " << sockfd_clnt << " has closed socket, closing this client..." << std::endl;
            close(sockfd_clnt);
            break;
        }
    }
}

void Connection::setDelConnCallback(std::function<void(Socket *)> cb) {
    _delConnCallback = cb;
}
