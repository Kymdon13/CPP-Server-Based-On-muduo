#include "include/TCP-Server.h"

#include <unistd.h>

#include <iostream>
#include <sstream>

#include "include/Acceptor.h"
#include "include/CurrentThread.h"
#include "include/EventLoop.h"
#include "include/Exception.h"
#include "include/TCP-Connection.h"
#include "include/ThreadPool.h"

#define MAX_CONN_ID 1000

TCPServer::TCPServer(const char *ip, const int port) {
  main_reactor_ = std::make_unique<EventLoop>();

  /**
   * init main reactor as Acceptor
   */
  acceptor_ = std::make_unique<Acceptor>(main_reactor_.get(), ip, port);

  /**
   * set Acceptor::on_new_connection_callback_
   * The call hierarchy is:
   * Channel::read_callback_()->Acceptor::on_new_connection_callback_()
   */
  acceptor_->OnNewConnection([this](int fd) {
    if (fd == -1) {
      WarnIf(true, "Acceptor::on_new_connection_callback_(): fd == -1");
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
      main_reactor_->CallOrQueue([this, conn]() {
        std::cout << "tid-" << CurrentThread::gettid() << ": TCPServer::HandleClose" << std::endl;
        // close the TCPConnection
        int fd = conn->GetFD();
        auto it = connection_map_.find(fd);
        if (it == connection_map_.end()) {
          std::stringstream ss;
          ss << "Connection::on_close_callback_(): can not find fd: " << fd << " in connection_map_";
          WarnIf(true, ss.str().c_str());
          return;
        }
        connection_map_.erase(fd);
        // remove the channel from the system epoll
        conn->GetEventLoop()->DeleteChannel(conn->GetChannel());
      });
    });
    // set TCPConnecion::on_connection_callback_. called in
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
      std::thread::hardware_concurrency() - 1;  // minus 1 cuz the main_reactor_ has taken one thread
  thread_pool_ = std::make_unique<ThreadPool>(hardware_concurrency);
}

void TCPServer::Start() {
  // dispatch the sub reactors to the threads
  thread_pool_->Init();

  main_reactor_->Loop();
}

void TCPServer::OnConnection(std::function<void(std::shared_ptr<TCPConnection>)> const &func) {
  on_connection_callback_ = std::move(func);
}
void TCPServer::OnMessage(std::function<void(std::shared_ptr<TCPConnection>)> const &func) {
  on_message_callback_ = std::move(func);
}
