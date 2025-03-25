
#include <string.h>
#include <unistd.h>

#include <cstdio>
#include <functional>
#include <iostream>
#include <string>

#include "../src/include/Buffer.h"
#include "../src/include/Socket.h"
#include "../src/include/ThreadPool.h"
#include "../src/include/Util.h"

void oneClient(int msgs, int wait) {
  Socket *sock = new Socket();  // blocking socket
  InetAddr *addr = new InetAddr("127.0.0.1", 8888);
  sock->connect(addr);

  int sockfd = sock->getFd();

  Buffer *sendBuffer = new Buffer();
  Buffer *readBuffer = new Buffer();

  sleep(wait);
  int count = 0;
  while (count < msgs) {  // send count times
    // set the msg
    sendBuffer->clear();
    const char *msg = "I'm client!";
    sendBuffer->append(msg, strlen(msg));

    // send the msg
    ssize_t write_bytes = write(sockfd, sendBuffer->GetBuffer(), sendBuffer->size());
    if (write_bytes == -1) {
      printf("socket already disconnected, can't write any more!\n");
      break;
    }

    // recv the msg from server
    size_t already_read = 0;
    char buf[1024];
    while (true) {
      memset(&buf, 0, sizeof(buf));
      ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
      if (read_bytes > 0) {
        readBuffer->append(buf, read_bytes);
        already_read += read_bytes;
      } else if (read_bytes == 0) {
        printf("server disconnected!\n");
        exit(EXIT_SUCCESS);
      }
      if (already_read >= sendBuffer->size()) {
        printf("count: %d, message from server: %s\n", ++count, readBuffer->GetBuffer());
        break;
      }
    }
    readBuffer->clear();
  }
  delete sock;
  delete addr;
  delete sendBuffer;
  delete readBuffer;
}

int main(int argc, char *argv[]) {
  using std::__cxx11::stoi;
  int threads = 100;
  int msgs = 100;
  int wait = 0;
  int o;
  const char *optstring = "t:m:w:";
  while ((o = getopt(argc, argv, optstring)) != -1) {
    switch (o) {
      case 't':
        threads = stoi(optarg);
        break;
      case 'm':
        msgs = stoi(optarg);
        break;
      case 'w':
        wait = stoi(optarg);
        break;
      case '?':
        printf("error optopt: %c\n", optopt);
        printf("error opterr: %d\n", opterr);
        break;
    }
  }

  ThreadPool *pool = new ThreadPool(threads);
  std::function<void()> func = std::bind(oneClient, msgs, wait);
  for (int i = 0; i < threads; ++i) pool->Add(func);

  std::cout << "this is a waiting point for all threads to complete their "
               "tasks, make sure all threads are done and"
            << std::endl;
  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  delete pool;
  return 0;
}
