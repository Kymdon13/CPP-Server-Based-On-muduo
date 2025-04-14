#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "base/CurrentThread.h"
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

const std::string html = " <font color=\"red\">This is html!</font> ";

void HTTPResponseCallback(const HTTPRequest *req, HTTPResponse *res) {
  if (req->GetMethod() != HTTPRequest::HTTPMethod::GET) {
    res->SetStatus(HTTPResponse::HTTPStatus::BadRequest);
    res->SetClose(true);
    return;
  }
  std::string url = req->GetUrl();
  if (url == "/") {
    res->SetStatus(HTTPResponse::HTTPStatus::OK);
    res->SetContentType(HTTPResponse::HTTPContentType::text_html);
    res->SetBody(html);
  } else if (url == "/hello") {
    res->SetStatus(HTTPResponse::HTTPStatus::OK);
    res->SetContentType(HTTPResponse::HTTPContentType::text_plain);
    res->SetBody("hello world\n");
  } else if (url == "/favicon.ico") {
    res->SetStatus(HTTPResponse::HTTPStatus::OK);
  } else {
    res->SetStatus(HTTPResponse::HTTPStatus::NotFound);
    res->SetBody("Sorry Not Found\n");
    res->SetClose(true);
  }
  return;
}

int main() {
  std::shared_ptr<AsyncLogging> async_logging = AsyncLogging::Init("log/single-client");
  async_logging->start();

  EventLoop *loop = new EventLoop();
  HTTPServer *server = new HTTPServer(loop, "0.0.0.0", 5000);
  server->OnMessage(HTTPResponseCallback);
  server->Start();

  delete server;
  delete loop;
  return 0;
}
