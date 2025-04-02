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
#include "tcp/Buffer.h"
#include "tcp/EventLoop.h"
#include "tcp/Poller.h"
#include "tcp/TCP-Connection.h"
#include "tcp/TCP-Server.h"
#include "tcp/Thread.h"
#include "tcp/ThreadPool.h"

const std::string html = " <font color=\"red\">This is html!</font> ";

void HTTPResponseCallback(const HTTPRequest *req, HTTPResponse *res) {
  if (req->GetMethod() != HTTPMethod::GET) {
    res->SetStatus(HTTPStatus::BadRequest);
    res->SetClose(true);
    return;
  }
  std::string url = req->GetUrl();
  if (url == "/") {
    res->SetStatus(HTTPStatus::OK);
    res->SetContentType(HTTPContentType::text_html);
    res->SetBody(html);
  } else if (url == "/hello") {
    res->SetStatus(::HTTPStatus::OK);
    res->SetContentType(HTTPContentType::text_plain);
    res->SetBody("hello world\n");
  } else if (url == "/favicon.ico") {
    res->SetStatus(HTTPStatus::OK);
  } else {
    res->SetStatus(HTTPStatus::NotFound);
    res->SetBody("Sorry Not Found\n");
    res->SetClose(true);
  }
  return;
}

int main() {
  HTTPServer *server = new HTTPServer("127.0.0.1", 5000);
  server->SetResponseCallback(HTTPResponseCallback);
  server->Start();

  delete server;
  return 0;
}
