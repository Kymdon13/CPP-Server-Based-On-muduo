#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <iostream>

#include "util.h"
#include "Epoll.h"
#include "InetAddr.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Server.h"

int main() {
    EventLoop *loop = new EventLoop();
    Server *server = new Server(loop);
    loop->loop();
    return 0;
}
