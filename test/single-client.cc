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

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_port = htons(5000);

  errorif(connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

  std::string http_request =
      "GET /hello?a=2&b=456  HTTP/1.1\r\n"
      "Host: 127.0.0.1:1234\r\n"
      "Connection: keep-alive\r\n"
      "Cache-Control: max-age=0\r\n"
      "sec-ch-ua: \"Google Chrome\";v=\"113\", \"Chromium\";v=\"113\", \"Not-A.Brand\";v=\"24\"\r\n"
      "sec-ch-ua-mobile: ?0\r\n"
      "sec-ch-ua-platform: \"Linux\"\r\n"
      "Upgrade-Insecure-Requests: 1\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/113.0.0.0 "
      "Safari/537.36\r\n"
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/"
      "signed-exchange;v=b3;q=0.7\r\n"
      "Sec-Fetch-Site: none\r\n"
      "Sec-Fetch-Mode: navigate\r\n"
      "Sec-Fetch-User: ?1\r\n"
      "Sec-Fetch-Dest: document\r\n"
      "Accept-Encoding: gzip, deflate, br\r\n"
      "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,zh-TW;q=0.7\r\n"
      "Cookie: "
      "username-127-0-0-1-8888=\"2|1:0|10:1681994652|23:username-127-0-0-1-8888|44:"
      "Yzg5ZjA1OGU0MWQ1NGNlMWI2MGQwYTFhMDAxYzY3YzU=|6d0b051e144fa862c61464acf2d14418d9ba26107549656a86d92e079ff033ea\";"
      " _xsrf=2|dd035ca7|e419a1d40c38998f604fb6748dc79a10|168199465\r\n"
      "Content-Length: 100\r\n"
      "\r\n"
      "";
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

    // send http
    write_bytes = write(sockfd, http_request.c_str(), http_request.size());

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

  errorif(connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

  std::string http_request =
      "GET /cat.jpg HTTP/1.1\r\n"
      "Host: 127.0.0.1:1234\r\n"
      "Connection: keep-alive\r\n"
      "Cache-Control: max-age=0\r\n"
      "sec-ch-ua: \"Google Chrome\";v=\"113\", \"Chromium\";v=\"113\", \"Not-A.Brand\";v=\"24\"\r\n"
      "sec-ch-ua-mobile: ?0\r\n"
      "sec-ch-ua-platform: \"Linux\"\r\n"
      "Upgrade-Insecure-Requests: 1\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/113.0.0.0 "
      "Safari/537.36\r\n"
      "Accept: "
      "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/"
      "signed-exchange;v=b3;q=0.7\r\n"
      "Sec-Fetch-Site: none\r\n"
      "Sec-Fetch-Mode: navigate\r\n"
      "Sec-Fetch-User: ?1\r\n"
      "Sec-Fetch-Dest: document\r\n"
      "Accept-Encoding: gzip, deflate, br\r\n"
      "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,zh-TW;q=0.7\r\n"
      "Cookie: "
      "username-127-0-0-1-8888=\"2|1:0|10:1681994652|23:username-127-0-0-1-8888|44:"
      "Yzg5ZjA1OGU0MWQ1NGNlMWI2MGQwYTFhMDAxYzY3YzU=|6d0b051e144fa862c61464acf2d14418d9ba26107549656a86d92e079ff033ea\";"
      " _xsrf=2|dd035ca7|e419a1d40c38998f604fb6748dc79a10|168199465\r\n"
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
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // wait for some time
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

int main() { http_client_header(); }
