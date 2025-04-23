#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/CurrentThread.h"
#include "base/FileUtil.h"
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

namespace fs = std::filesystem;
const fs::path g_staticPath = "/home/wzy/code/cpp-server/static/";

int main() {
  // std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::init("log/single-client");
  // async_logging->start();

  std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
  HTTPServer* server = new HTTPServer(loop.get(), "0.0.0.0", 5000, g_staticPath);

  server->onResponse([server](const HTTPRequest* req, HTTPResponse* res) {
    using Status = HTTPResponse::Status;
    using ContentType = HTTPResponse::ContentType;
    std::string url = req->url();
    LOG_TRACE << "HTTP request: " << '"' << url << '"';
    if (req->method() == HTTPRequest::Method::GET) {
      if (url == "/") {
        res->setStatus(Status::OK);
        res->setContentType(ContentType::text_html);
        // FIXME(wzy) getFile can not input rel path directly
        res->setBody(server->getFile(g_staticPath.string() + "linux_cmd.html"));
      } else if (server->findStaticFile(url)) {
        res->setStatus(Status::OK);
        if (fs::path(url).extension().string() == ".css") {
          res->setContentType(ContentType::text_css);
        } else if (fs::path(url).extension().string() == ".js") {
          res->setContentType(ContentType::text_js);
        } else if (fs::path(url).extension().string() == ".png") {
          res->setContentType(ContentType::image_png);
        } else if (fs::path(url).extension().string() == ".jpg") {
          res->setContentType(ContentType::image_jpg);
        } else if (fs::path(url).extension().string() == ".gif") {
          res->setContentType(ContentType::image_gif);
        } else if (fs::path(url).extension().string() == ".ico") {
          res->setContentType(ContentType::image_ico);
        } else if (fs::path(url).extension().string() == ".md") {
          res->setContentType(ContentType::text_md);
        } else {
          res->setContentType(ContentType::text_plain);
        }

        std::shared_ptr<Buffer> buf = server->getFile(g_staticPath.string() + url);
        if (buf == nullptr) {
          res->setStatus(Status::NotFound);
          res->setClose(true);
          return;
        }
        res->setBody(buf);
      } else {
        res->setStatus(Status::NotFound);
        res->setClose(true);
      }
    } else {
      res->setStatus(Status::NotImplemented);
      res->setClose(true);
    }
  });

  server->start();

  return 0;
}
