#pragma once

#include <stdio.h>
#include <sys/inotify.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "HTTP-Context.h"
#include "base/FileUtil.h"
#include "base/common.h"
#include "tcp/Channel.h"
#include "tcp/TCP-Server.h"

class TCPConnection;
class HTTPRequest;
class HTTPResponse;
class HTTPConnection;
class EventLoop;

class HTTPServer {
 private:
  EventLoop* loop_;
  std::unique_ptr<TCPServer> tcpServer_;
  std::unique_ptr<FileLRU> fileCache_;
  std::unique_ptr<Channel> inotifyChannel_;
  std::function<void(const HTTPRequest*, HTTPResponse*)> on_response_callback_;

 public:
  DISABLE_COPYING_AND_MOVING(HTTPServer);
  /// @brief
  /// @param loop
  /// @param ip
  /// @param port
  /// @param staticPath must be absolute path
  HTTPServer(EventLoop* loop, const char* ip, const int port, const std::filesystem::path& staticPath);
  ~HTTPServer() {}

  std::shared_ptr<Buffer> getFile(const std::filesystem::path& rel_or_abs_path) {
    return fileCache_->getFile(rel_or_abs_path);
  }

  void onResponse(std::function<void(const HTTPRequest*, HTTPResponse*)> cb) { on_response_callback_ = std::move(cb); }

  void start() { tcpServer_->start(); }
};
