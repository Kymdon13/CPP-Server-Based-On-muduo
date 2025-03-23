#include "Connection.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "util.h"

#include <unistd.h>
#include <string.h>
#include <iostream>

#define BUFFER_SIZE 1024

void Connection::_send(int sockfd) {
    size_t data_size = _readBuffer->size();
    size_t data_left = data_size; // how many bytes of data left in the buffer waiting to be sent
    char buffer[data_size];
    strcpy(buffer, _readBuffer->c_str());
    while (data_left > 0) {
        ssize_t bytes_write = write(sockfd, (buffer+data_size-data_left), data_left);
        if (bytes_write == -1 && errno == EAGAIN) break; // the send buffer of the 'socket' is full
        data_left -= bytes_write;
    }
}

Connection::Connection(EventLoop *el, Socket *sock, __flag_t flag) : _el(el),
                                                                     _sock(sock),
                                                                     _ch(nullptr),
                                                                     _readBuffer(nullptr)
{
    _ch = new Channel(_el, _sock->getFd());
    _ch->enableReading();
    _ch->enableEPOLLET();
    // bind callback to _ch according to flag
    std::function<void()> cb;
    if (flag & FLAGECHO) { // TODO use switch() instead of if() cause you can only bind one callback, set 2 flags, one for readCallback, one for writeCallback
        cb = std::bind(&Connection::echoCallback, this, _sock);
    } else if (flag & FLAGPRINT) {
        cb = std::bind(&Connection::printCallback, this, _sock);
    }
    _ch->setReadCallback(cb);
    _readBuffer = new Buffer();
}

Connection::~Connection() {
    delete _sock;
    delete _ch;
    delete _readBuffer;
}

void Connection::printCallback(Socket *sock) {
    char buffer[BUFFER_SIZE]; // buffer to store received data
    memset(buffer, 0, sizeof(buffer));
    size_t bytes_read = sock->readSync(buffer, sizeof(buffer));
    std::cerr << bytes_read << " bytes received from server:" << buffer << std::endl;
}

void Connection::echoCallback(Socket *sock) {
    char buffer[BUFFER_SIZE]; // buffer to store received data
    ssize_t bytes_read;
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // clear the buffer
        // receives data from the socket and stores it in the _readBuffer
        // then handle the received data
        bytes_read = read(sock->getFd(), buffer, sizeof(buffer));
        if (bytes_read > 0) {
            _readBuffer->append(buffer, bytes_read); // store it in the _readBuffer temporarily
        } else if (bytes_read == -1) {
            if (errno == EAGAIN) {
                // has read all the data into the _readBuffer
                std::cout << _readBuffer->size() << " bytes received from client:" << _readBuffer->c_str() << std::endl;
                std::cout << "client " << sock->getFd() << " done reading" << std::endl;
                // until we reach the end of the message, write data to client
                _send(sock->getFd());
                _readBuffer->clear();
                break;
            } else if (errno == EINTR) {
                std::cout << "interrupt by signal, continue reading..." << std::endl;
                continue;
            } else { // error
                _delConnCallback(_sock);
                break;
            }
        } else if (bytes_read == 0) { // client has closed the socket
            std::cout << "client " << sock->getFd() << " disconnected" << std::endl;
            _delConnCallback(_sock);
            break;
        }
    }
}

void Connection::setDelConnCallback(std::function<void(Socket*)> cb) {
    _delConnCallback = cb;
}
