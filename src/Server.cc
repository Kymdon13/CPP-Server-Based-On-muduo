#include "Server.h"

#include "util.h"
#include "Socket.h"
#include "InetAddr.h"
#include "Channel.h"

#include <fcntl.h>
#include <iostream>
#include <functional>
#include <string.h>
#include <unistd.h>

#define READ_BUFFER_SIZE 1024

Server::Server(EventLoop *loop) : _loop(loop) {
    // create a socket
    Socket *sock_serv = new Socket();
    // create an address
    InetAddr *addr_serv = new InetAddr("127.0.0.1", 8888);
    // set the socket to non-blocking mode
    sock_serv->setNonBlocking();
    // bind the socket to the address
    sock_serv->bind(addr_serv);
    // listen for incoming connections
    sock_serv->listen();

    Channel *ch_serv = new Channel(_loop, sock_serv->getFd());
    // set the callback function for the channel
    std::function<void()> cb = std::bind(&Server::handleNewConnEvent, this, sock_serv);
    ch_serv->setCallback(cb);
    // enable reading on the channel
    ch_serv->enableReading();
}

Server::~Server() {}

void Server::handleReadEvent (int sockfd_clnt) {
    char buffer[64]; // buffer to store received data
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // clear the buffer
        // receives data from the socket and stores it in the buffer
        // then handle the received data
        ssize_t bytes_read = read(sockfd_clnt, buffer, sizeof(buffer));
        if (bytes_read > 0) { // success
            std::cout << strlen(buffer) << " bytes received from client:" << buffer << std::endl;
        } else if (bytes_read == -1 && errno == EINTR) { // interrupted by signal, continue reading
            continue;
        } else if (bytes_read == -1 && (errno == EAGAIN || errno == O_NONBLOCK)) { // no data
            std::cout << "client " << sockfd_clnt << " done reading" << std::endl;
            break;
        } else if (bytes_read == -1) { // error
            close(sockfd_clnt);
            break;
        } else if (bytes_read == 0) { // client has closed the socket
            std::cout << "client " << sockfd_clnt << " has closed socket , closing this client..." << std::endl;
            close(sockfd_clnt);
            break;
        }
        // write data to client
        write(sockfd_clnt, buffer, bytes_read);
    }
}

void Server::handleNewConnEvent(Socket *sock_serv) {
    // FIXME (mem leak) create an address for the client
    InetAddr *addr_clnt = new InetAddr();

    // FIXME (mem leak) accept the connection
    Socket *sock_clnt = new Socket(sock_serv->accept(addr_clnt));
    sock_clnt->setNonBlocking();

    std::cout << "new client fd: " << sock_clnt->getFd()
    << " IP: " << inet_ntoa(addr_clnt->_addr.sin_addr)
    << " Port: " << ntohs(addr_clnt->_addr.sin_port) << std::endl;

    // FIXME (seems channel will never be deleted)
    Channel *ch_clnt = new Channel(_loop, sock_clnt->getFd());

    // set the callback function for the channel
    std::function<void()> cb = std::bind(&Server::handleReadEvent, this, sock_clnt->getFd());
    ch_clnt->setCallback(cb);
    // enable reading on the channel
    ch_clnt->enableReading();
}
