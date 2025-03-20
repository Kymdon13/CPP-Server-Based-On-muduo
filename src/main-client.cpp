#include "util.h"
#include "Socket.h"
#include "InetAddr.h"
#include "EventLoop.h"
#include "Server.h"
#include "Connection.h"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <functional>

#define BUFFER_SIZE 1024

int main() {
    Socket *sock = new Socket(); // sync socket
    InetAddr *addr = new InetAddr("127.0.0.1", 8888);

    // connect to server
    sock->connect(addr);

    EventLoop *loop = new EventLoop();

    // create Connection instance and bind it to epoll
    Connection *conn = new Connection(loop, sock, FLAGPRINT);

    /* buffer */
    char buffer[BUFFER_SIZE];
    while (!sock->getIsClosed()) {
        // clear the buffer
        memset(buffer, 0, sizeof(buffer));

        // get user input
        std::cout << "input: "; std::cin >> buffer;

        // write data to server
        ssize_t bytes_write = sock->writeSync(buffer, strlen(buffer));
        std::cerr << bytes_write << " bytes sent to server" << std::endl;

        // block on epoll_wait(), true means just handle once
        loop->loop(true);
    }

    delete conn;
}
