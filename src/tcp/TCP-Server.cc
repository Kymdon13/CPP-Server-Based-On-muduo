#include "TCP-Server.h"

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

TCPServer::TCPServer(EventLoop *loop, const char *ip, const int port) : loop_(loop) {
  /**
   * init main reactor as Acceptor
   */
  acceptor_ = std::make_unique<Acceptor>(loop_, ip, port);

  /**
   * set Acceptor::on_new_connection_callback_
   * The call hierarchy is:
   * Channel::read_callback_()->Acceptor::on_new_connection_callback_()
   */
  acceptor_->OnNewConnection([this](int fd) {
    if (fd == -1) {
      LOG_ERROR << "TCPServer::TCPServer, fd == -1";
      return;
    }
    // create TCPConnection (!!!note that TCPConnection must be held by a shared_ptr to enable the shared_from_this()
    // method), and dispatch it to a random subReactor
    std::shared_ptr<TCPConnection> conn =
        std::make_shared<TCPConnection>(thread_pool_->GetSubReactor(), fd, next_connection_id_);

    /**
     * set TCPConnecion::on_close_callback_
     */
    conn->OnClose([this](std::shared_ptr<TCPConnection> conn) {
      loop_->CallOrQueue([this, conn]() {
        // developer defined close function, used to free self-defined resources
        on_close_callback_(conn);

        std::cout << "tid-" << CurrentThread::gettid() << ": TCPServer::HandleClose" << std::endl;
        // remove the TCPConnection from the connection_map_
        int fd = conn->GetFD();
        auto it = connection_map_.find(fd);
        if (it == connection_map_.end()) {
          LOG_ERROR << "TCPServer::TCPServer, can not find fd: \"" << fd << "\" in connection_map_";
          return;
        }
        connection_map_.erase(fd);
        // remove the channel from the system epoll
        conn->GetChannel()->removeSelf();
      });
    });
    /**
     * set TCPConnecion's other callbacks
     */
    // set TCPConnecion::on_connection_callback_, will be called in TCPConnection::EnableConnection
    conn->OnConnection(on_connection_callback_);
    // set TCPConnecion::on_message_callback_
    conn->OnMessage(on_message_callback_);
    // update the connection_map_
    connection_map_[fd] = conn;
    // update next_connection_id_
    next_connection_id_ = (++next_connection_id_) % MAX_CONN_ID;
    // enable the connection
    conn->EnableConnection();
  });

  /**
   * create ThreadPool
   */
  unsigned int hardware_concurrency =
      std::thread::hardware_concurrency() - 2;  // leave 2 threads for main reactor and AsyncLogging
  thread_pool_ = std::make_unique<ThreadPool>(loop_, hardware_concurrency);
}

void TCPServer::Start() {
  // dispatch the sub reactors to the threads
  thread_pool_->Init();
  // start looping
  std::cout << "[main thread] - EventLoop start looping..." << std::endl;
  loop_->Loop();
}

void TCPServer::OnConnection(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_connection_callback_ = std::move(func);
}
void TCPServer::OnMessage(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_message_callback_ = std::move(func);
}
void TCPServer::OnClose(std::function<void(std::shared_ptr<TCPConnection>)> func) {
  on_close_callback_ = std::move(func);
}
