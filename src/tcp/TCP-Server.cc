#include "TCP-Server.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "log/Logger.h"
#include "tcp/Acceptor.h"
#include "tcp/Channel.h"
#include "tcp/EventLoop.h"
#include "tcp/TCP-Connection.h"
#include "tcp/ThreadPool.h"

#define MAX_CONN_ID 1000

TCPServer::TCPServer(EventLoop* loop, const char* ip, const int port) : loop_(loop) {
  /**
   * init main reactor as Acceptor
   */
  acceptor_ = std::make_unique<Acceptor>(loop_, ip, port);

  /**
   * set Acceptor::on_new_connection_callback_
   * The call hierarchy is:
   * Channel::read_callback_()->Acceptor::on_new_connection_callback_()
   */
  acceptor_->onNewConnection([this](int fd) {
    if (fd == -1) {
      LOG_ERROR << "TCPServer::TCPServer, fd == -1";
      return;
    }
    // create TCPConnection (!!!note that TCPConnection must be held by a shared_ptr to enable the shared_from_this()
    // method), and dispatch it to a random subReactor
    std::shared_ptr<TCPConnection> conn = std::make_shared<TCPConnection>(threadPool_->getSubReactor(), fd, next_id_);

    /**
     * set TCPConnecion::on_close_callback_
     */
    conn->onClose([this](std::shared_ptr<TCPConnection> conn) {
      loop_->callOrQueue([this, conn]() {
        // developer defined close function, used to free self-defined resources
        if (on_close_callback_) {
          on_close_callback_(conn);
        }
        int fd = conn->fd();

        // print out peer's info
        struct sockaddr_in addr_peer;
        socklen_t addrlength_peer = sizeof(addr_peer);
        getpeername(fd, (struct sockaddr*)&addr_peer, &addrlength_peer);
        std::cout << "tid-" << CurrentThread::gettid() << ' ' << "[fd#" << fd << "] " << inet_ntoa(addr_peer.sin_addr)
                  << ':' << ntohs(addr_peer.sin_port) << " Disconnected" << std::endl;

        // remove the TCPConnection from the conn_map_
        auto it = conn_map_.find(fd);
        if (it == conn_map_.end()) {
          LOG_ERROR << "TCPServer::TCPServer, can not find fd: \"" << fd << "\" in conn_map_";
          return;
        }
        conn_map_.erase(fd);

        // remove the channel from the system epoll
        conn->channel()->removeSelf();
      });
    });
    /**
     * set TCPConnecion's other callbacks
     */
    // set TCPConnecion::on_connection_callback_, will be called in TCPConnection::enableConnection
    conn->onConnection(on_connection_callback_);
    // set TCPConnecion::on_message_callback_
    conn->onMessage(on_message_callback_);
    // update the conn_map_
    conn_map_[fd] = conn;
    // update next_id_
    next_id_ = (++next_id_) % MAX_CONN_ID;
    // enable the connection
    conn->enableConnection();
  });

  /**
   * create ThreadPool
   */
  unsigned int hardware_concurrency =
      std::thread::hardware_concurrency() - 2;  // leave 2 threads for main reactor and AsyncLogging
  threadPool_ = std::make_unique<ThreadPool>(loop_, hardware_concurrency);
}

void TCPServer::start() {
  // dispatch the sub reactors to the threads
  threadPool_->init();
  // start looping
  std::cout << "[main thread] - EventLoop start looping..." << std::endl;
  loop_->loop();
}

void TCPServer::onConnection(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_connection_callback_ = std::move(func);
}
void TCPServer::onMessage(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_message_callback_ = std::move(func);
}
void TCPServer::onClose(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_close_callback_ = std::move(func);
}
