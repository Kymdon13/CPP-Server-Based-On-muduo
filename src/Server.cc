#include "Server.h"

#include "util.h"
#include "Socket.h"
#include "InetAddr.h"
#include "Channel.h"
#include "Acceptor.h"

#include <fcntl.h>
#include <iostream>
#include <functional>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

Server::Server(EventLoop *el) : _el(el), _acceptor(nullptr) {
    _acceptor = new Acceptor(_el);
    std::function<void(Socket*)> cb = std::bind(&Server::handleNewConnEvent, this, std::placeholders::_1);
    _acceptor->setNewConnCallback(cb);
}

Server::~Server() {
    delete _acceptor;
}

void Server::handleReadEvent (int sockfd_clnt) {
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

void Server::handleNewConnEvent(Socket *sock_serv) {
    // FIXME (mem leak) create an address for the client
    InetAddr *addr_clnt = new InetAddr();

    // FIXME (mem leak) accept the connection
    Socket *sock_clnt = new Socket(sock_serv->accept(addr_clnt));
    sock_clnt->setNonBlocking();

    std::cout << "new client fd: " << sock_clnt->getFd() <<
        " IP: " << inet_ntoa(addr_clnt->_addr.sin_addr) <<
        " Port: " << ntohs(addr_clnt->_addr.sin_port) << std::endl;

    // FIXME (seems channel will never be deleted)
    Channel *ch_clnt = new Channel(_el, sock_clnt->getFd());

    // set the callback function for the channel
    std::function<void()> cb = std::bind(&Server::handleReadEvent, this, sock_clnt->getFd());
    ch_clnt->setCallback(cb);
    // enable reading on the channel
    ch_clnt->enableReading();
}
