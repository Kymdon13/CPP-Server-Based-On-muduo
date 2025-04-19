#include <assert.h>

#include <iostream>
#include <string>

#include "tcp/Buffer.h"

void basicBuffer() {
  Buffer buffer(5);
  buffer.append("hello world", 11);  // expand to 8 + 11
  buffer.retrieve(5);                // readerIndex_ = 8 + 5
  buffer.append("hello", 5);         // trigger copy, readerIndex_ = 8, writerIndex_ = 8 + 11
  buffer.retrieve(5);                // readerIndex_ = 8 + 5
  buffer.append("hello", 5);         // trigger copy, readerIndex_ = 8, writerIndex_ = 8 + 6
  buffer.retrieve(5);                // readerIndex_ = 8 + 5
  buffer.append("hello", 5);         // trigger copy, readerIndex_ = 8, writerIndex_ = 8 + 6
}

// void basicCycleBuffer() {
//     CycleBuffer cb(10);
//     cb.append("1234567890", 10);
//     cb.retrieve(3);
//     std::cout << "readableBytes: " << cb.readableBytes() << std::endl;
//     std::cout << "writableBytes: " << cb.writableBytes() << std::endl;
//     cb.append("abc", 3);
//     std::cout << "readableBytes: " << cb.readableBytes() << std::endl;
//     std::cout << "writableBytes: " << cb.writableBytes() << std::endl;
//     cb.retrieve(3);
//     auto rs = cb.peek();
//     for (int i = 0; i < rs.size(); ++i) {
//         std::cout << rs[i] << " ";
//     }
//     cb.retrieveAll();
// }

std::string test_string =
    "GET /hello?a=2&b=456 HTTP/1.1\r\n"
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

void benchmarkBuffer() {
  Buffer buffer(1024);
  // one iteration
  buffer.append(test_string.c_str(), test_string.size());
  const char* c = buffer.peek();
  // simulate data reading
  char tmp;
  for (size_t i = 0; i < buffer.readableBytes(); ++i) {
    tmp = *c;
    ++c;
  }
  buffer.retrieveAll();
}

int main() { return 0; }