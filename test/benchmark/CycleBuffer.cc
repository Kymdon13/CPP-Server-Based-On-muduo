#include <iostream>
#include <string>

#include <benchmark/benchmark.h>

#include "tcp/Buffer.h"

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
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,zh-TW;q=0.7\r\n"
    "\r\n";

void basicBuffer() {
  Buffer buffer(5);
  buffer.append("hello world", 11);  // expand to 8 + 11
  buffer.retrieve(5);                // readerIndex_ = 8 + 5
  buffer.append("hello", 5);         // trigger copy, readerIndex_ = 8, writerIndex_ = 8 + 11
  buffer.retrieve(5);                // readerIndex_ = 8 + 5
  buffer.append("hello", 5);         // trigger copy, readerIndex_ = 8, writerIndex_ = 8 + 6
}

Buffer buffer(1024);
static void BM_Buffer(benchmark::State& state) {
  int n = state.range(0);
  for (auto _ : state) {
    // iter n epoch
    for (int i = 0; i < n; ++i) {
      buffer.append(test_string.c_str(), test_string.size());
      const char* c = buffer.peek();
      // simulate data reading
      for (size_t i = 0; i < buffer.readableBytes(); ++i) {
        char tmp = *c;
        ++c;
      }
      buffer.retrieveAll();
    }
  }
}

void basicFuncCycleBuffer() {
  CycleBuffer cbuffer(10);

  cbuffer.append("1234567890", 10);
  cbuffer.retrieve(3);
  std::cout << "readableBytes: " << cbuffer.readableBytes() << std::endl;
  std::cout << "writableBytes: " << cbuffer.writableBytes() << std::endl;

  cbuffer.append("abc", 3);
  std::cout << "readableBytes: " << cbuffer.readableBytes() << std::endl;
  std::cout << "writableBytes: " << cbuffer.writableBytes() << std::endl;

  cbuffer.retrieve(3);

  auto readspan = cbuffer.peek();
  for (int i = 0; i < readspan.size(); ++i) {
    std::cout << readspan[i] << " ";
  }
  cbuffer.retrieveAll();

  std::cout << std::endl;
}

CycleBuffer cbuffer(1024);
static void BM_CycleBuffer(benchmark::State& state) {
  int n = state.range(0);
  for (auto _ : state) {
    // iter n epoch
    for (int i = 0; i < n; ++i) {
      cbuffer.append(test_string.c_str(), test_string.size());
      auto readspan = cbuffer.peek();
      // simulate data reading
      for (size_t i = 0; i < readspan.size(); ++i) {
        char tmp = readspan[i];
      }
      cbuffer.retrieveAll();
    }
  }
}

BENCHMARK(BM_Buffer)->RangeMultiplier(2)->Range(1 << 15, 1 << 20);
BENCHMARK(BM_CycleBuffer)->RangeMultiplier(2)->Range(1 << 15, 1 << 20);

BENCHMARK_MAIN();
