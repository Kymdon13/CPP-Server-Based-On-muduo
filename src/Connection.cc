#include "Connection.h"
#include "Socket.h"
#include "Channel.h"

#include <unistd.h>
#include <string.h>
#include <iostream>

#define BUFFER_SIZE 1024

Connection::Connection(EventLoop *el, Socket *sock) :
    _el(el),
    _sock(sock),
    _ch_clnt(nullptr)
{
    _ch_clnt = new Channel(_el, _sock->getFd());
    std::function<void()> cb = std::bind(&Connection::echoCallback, this, _sock->getFd());
    _ch_clnt->setCallback(cb);
    _ch_clnt->enableReading();
}

Connection::~Connection() {
    delete _sock;
    delete _ch_clnt;
}

void Connection::echoCallback(int sockfd_clnt) {
    char buffer[BUFFER_SIZE]; // buffer to store received data
    ssize_t bytes_read;
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // clear the buffer
        // receives data from the socket and stores it in the buffer
        // then handle the received data
        bytes_read = read(sockfd_clnt, buffer, sizeof(buffer));

        if (bytes_read > 0) { // read some data
            std::cout << strlen(buffer) << " bytes received from client:" << buffer << std::endl;
            // write data to client
            write(sockfd_clnt, buffer, bytes_read);
        } else if (bytes_read == -1) {
            if (errno == EAGAIN) { // has read all data and no data left
                std::cout << "client " << sockfd_clnt << " done reading" << std::endl;
                break;
            }
            // error
            close(sockfd_clnt);
            break;
        } else if (bytes_read == 0) { // client has closed the socket
            std::cout << "client " << sockfd_clnt << " has closed socket , closing this client..." << std::endl;
            close(sockfd_clnt);
            break;
        }
    }
}

void Connection::setDelConnCallback(std::function<void(Socket *)> cb) {
    _delConnCallback = cb;
}
