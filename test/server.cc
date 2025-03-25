#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "../src/include/Connection.h"
#include "../src/include/Epoll.h"
#include "../src/include/EventLoop.h"
#include "../src/include/Server.h"
#include "../src/include/Socket.h"
#include "../src/include/Util.h"

int main() {
  EventLoop *loop = new EventLoop();
  TCPServer *server = new TCPServer(loop);
  loop->Loop();
  delete server;
  delete loop;
  return 0;
}
