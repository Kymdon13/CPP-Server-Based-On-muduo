#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "../src/include/Connection.h"
#include "../src/include/EventLoop.h"
#include "../src/include/Server.h"
#include "../src/include/Socket.h"
#include "../src/include/Util.h"

#define BUFFER_SIZE 1024

int main() {
  Socket *sock = new Socket();  // sync socket
  InetAddr *addr = new InetAddr("127.0.0.1", 8888);

  // connect to server
  sock->connect(addr);

  EventLoop *loop = new EventLoop();

  // create TCPConnection instance and bind it to epoll
  TCPConnection *conn = new TCPConnection(loop, sock, FLAGPRINT);

  /* buffer */
  char buffer[BUFFER_SIZE];
  while (!sock->getIsClosed()) {
    // clear the buffer
    memset(buffer, 0, sizeof(buffer));

    // get user input
    std::cout << "input: ";
    std::cin >> buffer;

    // write data to server
    ssize_t bytes_write = sock->writeSync(buffer, strlen(buffer));
    std::cerr << bytes_write << " bytes sent to server" << std::endl;

    // block on epoll_wait(), true means just handle once
    loop->loopOnce();
  }

  delete conn;
  delete loop;
  delete addr;
  delete sock;
}
