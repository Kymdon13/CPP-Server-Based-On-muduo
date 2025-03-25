#include "TCP-Server.h"

#include <unistd.h>

#include <iostream>

#include "Acceptor.h"
#include "EventLoop.h"
#include "Exception.h"
#include "TCP-Connection.h"
#include "ThreadPool.h"

#define MAX_CONN_ID 1000

TCPServer::TCPServer(const char *ip, const int port) {
  main_reactor_ = std::make_unique<EventLoop>();
  acceptor_ = std::make_unique<Acceptor>(main_reactor_.get(), ip, port);

  acceptor_->SetNewConnectionCallback([this](int fd) { HandleNewConnection(fd); });

  unsigned int hardware_concurrency = std::thread::hardware_concurrency();
  thread_pool_ = std::make_unique<ThreadPool>(hardware_concurrency);

  // create n (n = hardware_concurrency) EventLoop, put them into sub_reactors_
  for (size_t i = 0; i < hardware_concurrency; ++i) {
    sub_reactors_.emplace_back(std::make_unique<EventLoop>());
  }
}

void TCPServer::Start() {
  for (size_t i = 0; i < sub_reactors_.size(); ++i) {
    EventLoop *el = sub_reactors_[i].get();
    thread_pool_->Add([el]() { el->Loop(); });
  }
  main_reactor_->Loop();
}

void TCPServer::OnConnect(std::function<void(TCPConnection *)> const &func) { on_connect_callback_ = std::move(func); }

void TCPServer::OnMessage(std::function<void(TCPConnection *)> const &func) { on_message_callback_ = std::move(func); }

void TCPServer::HandleClose(int fd) {
  auto it = connection_map_.find(fd);
  if (it == connection_map_.end()) {
    WarnIf(true, "HandleClose can not find fd in connection_map_");
    return;
  }
  TCPConnection *conn = connection_map_[fd];
  connection_map_.erase(fd);
  delete conn;
}

void TCPServer::HandleNewConnection(int fd) {
  if (fd == -1) {
    WarnIf(true, "HandleNewConnection: fd == -1");
    return;
  }
  std::cout << "New TCP fd: " << fd << std::endl;

  size_t which_sub_reactor = fd % sub_reactors_.size();

  TCPConnection *conn = new TCPConnection(sub_reactors_[which_sub_reactor].get(), fd, next_connection_id_);

  conn->SetOnCloseCallback([this](int fd) { HandleClose(fd); });

  conn->SetOnMessageCallback(on_message_callback_);

  next_connection_id_ = (next_connection_id_ + 1) % MAX_CONN_ID;
}
