#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>

#include "base/Exception.h"

#define BUFFER_SIZE 4096

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  ErrorIf(sockfd == -1, "socket create error");

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_port = htons(5000);

  ErrorIf(connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

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
      "zxcqp\r\nafjsdlkfjalskdjflaksdnflaskdnkclnasldca^%123jaklsncas*&*@&#07123hjnsd";

  bool flag = false;

  while (true) {
    ssize_t write_bytes;
    char buf[BUFFER_SIZE];  // 在这个版本，buf大小必须大于或等于服务器端buf大小，不然会出错，想想为什么？
    bzero(&buf, sizeof(buf));

    std::cout << "input:";
    std::cin >> buf;

    // send http
    if (!flag) {
      write_bytes = write(sockfd, http_request.c_str(), http_request.size());
      flag = true;
    } else {
      write_bytes = write(sockfd, buf, sizeof(buf));
    }

    if (write_bytes == -1) {
      printf("socket already disconnected, can't write any more!\n");
      break;
    }
    bzero(&buf, sizeof(buf));
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
    if (read_bytes > 0) {
      printf("message from server: %s\n", buf);
    } else if (read_bytes == 0) {
      printf("server socket disconnected!\n");
      close(sockfd);
      break;
    } else if (read_bytes == -1) {
      close(sockfd);
      ErrorIf(true, "socket read error");
    }
  }
  close(sockfd);
  return 0;
}
