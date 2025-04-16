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
  std::string url = req->GetUrl();
  LOG_TRACE << "HTTP request: " << '"' << url << '"';
  if (req->GetMethod() == HTTPRequest::HTTPMethod::GET) {
    if (url == "/") {
      res->SetStatus(HTTPResponse::HTTPStatus::OK);
      res->SetContentType(HTTPResponse::HTTPContentType::text_html);
      res->SetBody(FileUtil::ReadFile(HTTPResponse::GetStaticPath() + "index.html").data());
    } else if (url == "/cat") {
      res->SetStatus(HTTPResponse::HTTPStatus::OK);
      res->SetContentType(HTTPResponse::HTTPContentType::text_html);
      res->SetBody(FileUtil::ReadFile(HTTPResponse::GetStaticPath() + "cat.html").data());
    } else if (url == "/cat.jpg") {
      res->SetStatus(HTTPResponse::HTTPStatus::OK);
      res->SetContentType(HTTPResponse::HTTPContentType::image_jpeg);
      res->SetBody(FileUtil::ReadFile(HTTPResponse::GetStaticPath() + "cat.jpg", true).data());
    } else {
      res->SetStatus(HTTPResponse::HTTPStatus::NotFound);
      res->SetClose(true);
    }
  } else if (req->GetMethod() == HTTPRequest::HTTPMethod::POST) {
  } else {
    res->SetStatus(HTTPResponse::HTTPStatus::NotImplemented);
    res->SetClose(true);
  }
}

int main() {
  std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::Init("log/single-client");
  async_logging->start();

  EventLoop *loop = new EventLoop();
  HTTPServer *server = new HTTPServer(loop, "0.0.0.0", 5000);
  server->OnResponse(HTTPResponseCallback);
  server->Start();

  delete server;
  delete loop;
  return 0;
}
