#include <iostream>
#include <map>
#include <string>

#include "http/HTTP-Context.h"
#include "http/HTTP-Request.h"

int main() {
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

  std::cout << "http_request.size(): " << http_request.size() << std::endl;
  std::cout << "====================" << std::endl;

  HTTPContext *context = new HTTPContext();
  HTTPRequestParseState state = context->ParseRequest(http_request.c_str(), http_request.size());
  if (state == HTTPRequestParseState::COMPLETE) {
    std::cout << "Complete" << std::endl;
  } else {
    std::cout << "InComplete" << std::endl;
  }
  std::cout << "====================" << std::endl;
  HTTPRequest *request = context->GetHTTPRequest();

  std::cout << "method: " << request->GetMethodString() << std::endl << std::endl;

  std::cout << "url: " << request->GetUrl() << std::endl;
  std::cout << "request_params: " << std::endl;
  for (auto it : request->GetRequestParams()) {
    std::cout << "key: " << it.first << " | "
              << "value: " << it.second << std::endl;
  }

  std::cout << "protocol: " << request->GetProtocol() << std::endl << std::endl;

  std::cout << "version: " << request->GetVersionString() << std::endl << std::endl;

  std::cout << "headers: " << std::endl;
  for (auto it : request->GetHeaders()) {
    std::cout << "key: " << it.first << " | "
              << "value: " << it.second << std::endl;
  }

  std::cout << std::endl;

  std::cout << "body: " << request->GetBody() << std::endl;
}
