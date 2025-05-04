#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "base/CurrentThread.h"
#include "base/FileUtil.h"
#include "base/Exception.h"
#include "http/HTTP-Request.h"
#include "http/HTTP-Response.h"
#include "http/HTTP-Server.h"
#include "log/AsyncLogging.h"
#include "tcp/EventLoop.h"
#include "tcp/Poller.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"
#include "tcp/Thread.h"
#include "tcp/ThreadPool.h"
#include "base/json.hpp"

namespace fs = std::filesystem;
const fs::path g_staticPath = "/home/wzy/code/cpp-server/static/";

using HandleFunc = std::function<void(const HTTPRequest*, HTTPResponse*)>;
using json = nlohmann::json;

std::unordered_map<std::string, HandleFunc> g_routes;

void addRoute(const std::string& method, const std::string& path, HandleFunc func) {
  g_routes[method + path] = func;  // directly use method + path as the key
}

void setupRoutes(HTTPServer* server) {
  using Status = HTTPResponse::Status;
  using ContentType = HTTPResponse::ContentType;

  addRoute("GET", "/", [server](const HTTPRequest* req, HTTPResponse* res) {
    LOG_TRACE << "HTTP request: " << '"' << req->url() << '"';
    res->setStatus(Status::OK);
    res->setContentType(ContentType::text_html);
    res->setBody(server->getFile("/linux_cmd.html"));
  });

  addRoute("GET", "/download", [server](const HTTPRequest* req, HTTPResponse* res) {
    LOG_TRACE << "HTTP request: " << '"' << req->url() << '"';
    // Handle download logic here
    std::string url = req->url();
    std::shared_ptr<Buffer> buf;
    try {
      buf = server->getFile(url);
      if (buf == nullptr) {
        LOG_ERROR << "HTTPServer::onResponse, file not found";
        res->setClose(true);
        res->setStatus(Status::NotFound);
        return;
      }
      res->setBody(buf);
      res->setStatus(Status::OK);
      res->setContentType(ContentType::application_octet_stream);
      res->addHeader("Content-Disposition", "attachment; filename=" + fs::path(url).filename().string());
    } catch (const bigfile_error& e) {
      fs::path file_path = g_staticPath / fs::path(url).relative_path();
      int file_fd = ::open(file_path.c_str(), O_RDONLY);
      if (file_fd == -1) {
        LOG_ERROR << "TCPConnection::sendFile, open file failed: " << file_path;
        res->setClose(true);
        res->setStatus(Status::NotFound);
        return;
      }
      struct stat st;
      ::fstat(file_fd, &st);

      res->setBigFile(file_fd);
      res->setStatus(Status::OK);
      res->setContentType(ContentType::application_octet_stream);
      res->addHeader("Content-Length", std::to_string(st.st_size));
      res->addHeader("Content-Disposition", "attachment; filename=" + fs::path(url).filename().string());
    }
  });

  addRoute("POST", "/evaluate.json", [](const HTTPRequest* req, HTTPResponse* res) {
    std::cout << "=======================================================" << std::endl;
    for (auto it = req->headers().begin(); it != req->headers().end(); ++it) {
      std::cout << "Header: " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "=======================================================" << std::endl;
    json body = json::parse(req->body());
    std::cout << "version: " << body["version"].get<std::string>() << std::endl;
    std::cout << "optimize: " << body["optimize"].get<int>() << std::endl;
    std::cout << "code: " << body["code"].get<std::string>() << std::endl;
    std::cout << "edition: " << body["edition"].get<int>() << std::endl;

    std::fstream file("main.cc", std::ios::out | std::ios::trunc);  // trunc to clear or create the file
    file << "// This is a test file\n";
    file << "#include <iostream>\n";
    file << "#include <string>\n";
    file << body["code"].get<std::string>();

    res->setClose(true);
    res->setStatus(HTTPResponse::Status::OK);
  });

  addRoute("GET", "/*", [server](const HTTPRequest* req, HTTPResponse* res) {
    LOG_TRACE << "HTTP request: " << '"' << req->url() << '"';
    // Handle static file serving here
    std::string url = req->url();
    std::shared_ptr<Buffer> buf;
    try {
       buf = server->getFile(url);
       if (buf == nullptr) {
         LOG_ERROR << "HTTPServer::onResponse, file not found";
         res->setClose(true);
         res->setStatus(Status::NotFound);
         return;
       }
       res->setBody(buf);
       res->setStatus(Status::OK);
       res->setContentType(fs::path(url).extension().string());
    } catch (const bigfile_error& e) {
      fs::path file_path = g_staticPath / fs::path(url).relative_path();
      int file_fd = ::open(file_path.c_str(), O_RDONLY);
      if (file_fd == -1) {
        LOG_ERROR << "TCPConnection::sendFile, open file failed: " << file_path;
        res->setClose(true);
        res->setStatus(Status::NotFound);
        return;
      }
      struct stat st;
      ::fstat(file_fd, &st);

      res->setBigFile(file_fd);
      res->setStatus(Status::OK);
      res->setContentType(ContentType::application_octet_stream);
      res->addHeader("Content-Length", std::to_string(st.st_size));
      res->addHeader("Content-Disposition", "attachment; filename=" + fs::path(url).filename().string());
    }
  });
}

int main() {
  // std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::init("log/single-client");
  // async_logging->start();

  std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
  HTTPServer* server = new HTTPServer(loop.get(), "0.0.0.0", 5000, g_staticPath);

  // routing the request to the right handler
  server->onResponse([](const HTTPRequest* req, HTTPResponse* res) {
    auto it = g_routes.find(req->methodAsString() + req->url());
    if (it != g_routes.end()) {
      it->second(req, res);
    } else if (req->method() != HTTPRequest::Method::GET && req->method() != HTTPRequest::Method::POST) {
      res->setClose(true);
      res->setStatus(HTTPResponse::Status::NotImplemented);
    } else {  // fallback to default handler
      g_routes["GET/*"](req, res);
    }
  });

  setupRoutes(server);

  server->start();

  return 0;
}
