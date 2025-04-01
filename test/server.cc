#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "CurrentThread.h"
#include "EventLoop.h"
#include "Exception.h"
#include "Poller.h"
#include "TCP-Connection.h"
#include "TCP-Server.h"
#include "Thread.h"
#include "ThreadPool.h"

int main() {
  TCPServer *server = new TCPServer("127.0.0.1", 8888);

  server->OnMessage([](std::shared_ptr<TCPConnection> conn) {
    std::cout << "Message from client [fd#" << conn->GetFD() << "]: " << conn->GetReadBuffer() << std::endl;
    conn->Send(conn->GetReadBuffer());
  });
  server->OnConnection([](std::shared_ptr<TCPConnection> conn) {
    int fd_clnt = conn->GetFD();
    struct sockaddr_in addr_peer;
    socklen_t addrlength_peer = sizeof(addr_peer);
    getpeername(fd_clnt, (struct sockaddr *)&addr_peer, &addrlength_peer);

    std::cout << "tid-" << CurrentThread::gettid() << " Connection"
              << "[fd#" << fd_clnt << "]"
              << " from " << inet_ntoa(addr_peer.sin_addr) << ":" << ntohs(addr_peer.sin_port) << std::endl;
  });

  server->Start();

  delete server;
  return 0;
}
