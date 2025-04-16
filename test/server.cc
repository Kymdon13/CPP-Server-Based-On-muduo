#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
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

void HTTPResponseCallback(const HTTPRequest *req, HTTPResponse *res) {
  std::string url = req->url();
  LOG_TRACE << "HTTP request: " << '"' << url << '"';
  if (req->method() == HTTPRequest::Method::GET) {
    if (url == "/") {
      res->setStatus(HTTPResponse::Status::OK);
      res->setContentType(HTTPResponse::ContentType::text_html);
      res->setBody(FileUtil::ReadFile(HTTPResponse::staticPath() + "index.html").data());
    } else if (url == "/cat") {
      res->setStatus(HTTPResponse::Status::OK);
      res->setContentType(HTTPResponse::ContentType::text_html);
      res->setBody(FileUtil::ReadFile(HTTPResponse::staticPath() + "cat.html").data());
    } else if (url == "/cat.jpg") {
      res->setStatus(HTTPResponse::Status::OK);
      res->setContentType(HTTPResponse::ContentType::image_jpeg);
      res->setBody(FileUtil::ReadFile(HTTPResponse::staticPath() + "cat.jpg", true).data());
    } else {
      res->setStatus(HTTPResponse::Status::NotFound);
      res->setClose(true);
    }
  } else if (req->method() == HTTPRequest::Method::POST) {
  } else {
    res->setStatus(HTTPResponse::Status::NotImplemented);
    res->setClose(true);
  }
}

int main() {
  std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::init("log/single-client");
  async_logging->start();

  EventLoop *loop = new EventLoop();
  HTTPServer *server = new HTTPServer(loop, "0.0.0.0", 5000);
  server->onResponse(HTTPResponseCallback);
  server->start();

  delete server;
  delete loop;
  return 0;
}
