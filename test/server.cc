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
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../build/executor.grpc.pb.h"
#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "base/FileUtil.h"
#include "base/gRPC-ExecutorClient.hpp"
#include "base/json.hpp"
#include "http/HTTP-Request.h"
#include "http/HTTP-Response.h"
#include "http/HTTP-Server.h"
#include "log/AsyncLogging.h"
#include "log/Logger.h"
#include "tcp/EventLoop.h"
#include "tcp/Poller.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"
#include "tcp/Thread.h"
#include "tcp/ThreadPool.h"

namespace fs = std::filesystem;
const fs::path g_staticPath = "../static/";

using HandleFunc = std::function<void(const HTTPRequest*, HTTPResponse*)>;
using json = nlohmann::json;

// global variables
std::map<std::string, HandleFunc> g_routes;
std::unique_ptr<grpc::ExecutorClient> g_executor;

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

  /* allow redirect request from other website */
  addRoute("OPTIONS", "/evaluate.json", [](const HTTPRequest* req, HTTPResponse* res) {
    (void)req;
    res->setStatus(HTTPResponse::Status::NoContent);
    res->addHeader("access-control-allow-origin", "*");
    res->addHeader("access-control-allow-methods", "POST, GET, OPTIONS");
    res->addHeader("access-control-allow-headers", "Host, Origin, Referer, Connection, Content-Type, Content-Length");
  });

  addRoute("POST", "/evaluate.json", [](const HTTPRequest* req, HTTPResponse* res) {
    // std::cout << "=======================================================" << std::endl;
    // for (auto it = req->headers().begin(); it != req->headers().end(); ++it) {
    //   std::cout << "Header: " << it->first << ": " << it->second << std::endl;
    // }
    // std::cout << "=======================================================" << std::endl;
    json json_req;
    try {
      json_req = json::parse(req->body());
      // std::cout << "code: " << json_req["code"].get<std::string>() << std::endl;
      // std::cout << "edition: " << json_req["edition"].get<std::string>() << std::endl;
      // std::cout << "=======================================================" << std::endl;
    } catch (const std::exception& e) {
      LOG_ERROR << "JSON parse error: " << e.what();
      res->setClose(true);
      res->setStatus(HTTPResponse::Status::BadRequest);
      res->setBody(std::make_shared<Buffer>("Invalid JSON"));
      return;
    }

    /* handle bad cases */
    if (json_req.find("code") == json_req.end() || json_req.find("edition") == json_req.end()) {
      LOG_ERROR << "JSON parse error: code/edition not found";
      res->setClose(true);
      res->setStatus(HTTPResponse::Status::BadRequest);
      res->setBody(std::make_shared<Buffer>("Invalid JSON"));
      return;
    }
    std::string code = json_req["code"].get<std::string>();
    int edition = atoi(json_req["edition"].get<std::string>().c_str());
    if (code.empty()) {
      nlohmann::ordered_json json_res;
      json_res["result"] = "";
      json_res["error"] = "No code sent to the server!";
      res->setBody(std::make_shared<Buffer>(json_res.dump()));
      return;
    }
    if (code.size() > 10 * 1024) {
      nlohmann::ordered_json json_res;
      json_res["result"] = "";
      json_res["error"] = "Code is too long! Like over 10 * 1024 bytes that long!";
      res->setBody(std::make_shared<Buffer>(json_res.dump()));
      return;
    }
    if (edition != 11 && edition != 17 && edition != 20) {
      nlohmann::ordered_json json_res;
      json_res["result"] = "";
      json_res["error"] = "Invalid edition! Only support C++11, C++17 and C++20!";
      res->setBody(std::make_shared<Buffer>(json_res.dump()));
      return;
    }

    /* register callback. If you don't return earlier, this is where you end up */
    res->setCallback([code, edition](std::shared_ptr<TCPConnection> conn, HTTPResponse* res) {
      g_executor->GetResult(grpc::MakeReq(code, edition), conn, res);
    });
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
    } catch (const bigfile_error& e) {  // actually it's not an error, it's a jump out point
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
  std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::init("log/release", Logger::LogLevel::INFO);
  async_logging->start();

  std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
  HTTPServer* server = new HTTPServer(loop.get(), "0.0.0.0", 5000, g_staticPath);

  // gRPC server for executing code
  g_executor = std::make_unique<grpc::ExecutorClient>(
      grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

  // routing the request to the right handler
  server->onResponse([](const HTTPRequest* req, HTTPResponse* res) {
    auto it = g_routes.find(req->methodAsString() + req->url());
    if (it != g_routes.end()) {
      it->second(req, res);
    } else {  // fallback to default handler
      g_routes["GET/*"](req, res);
    }
  });

  // set up routes for HTTP requests
  setupRoutes(server);

  server->start();

  return 0;
}
