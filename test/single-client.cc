#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

#include "base/Exception.h"
#include "log/AsyncLogging.h"

#define BUFFER_SIZE 4096

// super long http body: 100 * 1000 bytes
void http_client_body() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  errorif(sockfd == -1, "socket create error");

  // ban Nagle
  int flag = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_port = htons(5000);

  errorif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

  std::string http_request =
      "GET /cat.jpg HTTP/1.1\r\n"
      "Content-Length: 100010\r\n"
      "\r\n"
      "helloworld";

  http_request.reserve(10 * 1000 * 10);
  for (int i = 0; i < 10 * 1000; ++i) {
    http_request += "helloworld";
  }

  while (true) {
    ssize_t write_bytes;
    char buf[BUFFER_SIZE];
    bzero(&buf, sizeof(buf));

    std::cout << "input:";
    std::cin >> buf;

    // send http, write twice
    size_t bytes_once = 60000;
    write_bytes = write(sockfd, http_request.c_str(), bytes_once);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait for some time
    write_bytes = write(sockfd, http_request.c_str() + bytes_once, http_request.size() - bytes_once);

    if (write_bytes == -1) {
      printf("socket already disconnected, can't write any more!\n");
      break;
    }
    bzero(&buf, sizeof(buf));
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
    if (read_bytes > 0) {
      printf("==========================\n");
      printf("message from server:\n%s\n", buf);
      printf("==========================\n");
    } else if (read_bytes == 0) {
      printf("server socket disconnected!\n");
      close(sockfd);
      break;
    } else if (read_bytes == -1) {
      close(sockfd);
      errorif(true, "socket read error");
    }
  }
  close(sockfd);
}

// super long http header
void http_client_header() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  errorif(sockfd == -1, "socket create error");

  // ban Nagle
  int flag = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_port = htons(5000);

  errorif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

  std::string http_request =
      "GET /cat.jpg HTTP/1.1\r\n"
      "Host: 127.0.0.1:1234\r\n"
      "Miao: No\r\n";

  // construct a super long http header
  http_request.reserve(10 * 1000 * 10 + 2);
  for (int i = 0; i < 5 * 1000; ++i) {
    http_request += "Miao: No\r\n";
  }

  http_request += "\r\n";

  while (true) {
    ssize_t write_bytes;
    char buf[BUFFER_SIZE];
    bzero(&buf, sizeof(buf));

    std::cout << "input:";
    std::cin >> buf;

    // send http, write once
    // write_bytes = write(sockfd, http_request.c_str(), http_request.size());

    // send http, write twice
    size_t bytes_once = 32741;
    write_bytes = write(sockfd, http_request.c_str(), bytes_once);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait for some time
    write_bytes = write(sockfd, http_request.c_str() + bytes_once, http_request.size() - bytes_once);

    if (write_bytes == -1) {
      printf("socket already disconnected, can't write any more!\n");
      break;
    }
    bzero(&buf, sizeof(buf));
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
    if (read_bytes > 0) {
      printf("==========================\n");
      printf("message from server:\n%s\n", buf);
      printf("==========================\n");
    } else if (read_bytes == 0) {
      printf("server socket disconnected!\n");
      close(sockfd);
      break;
    } else if (read_bytes == -1) {
      close(sockfd);
      errorif(true, "socket read error");
    }
  }
  close(sockfd);
}

int main() { http_client_body(); }
