#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#define BUFFER_SIZE 4096

int Socket(int port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) return sockfd;

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_port = htons(port);

  if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) return -1;
  return sockfd;
}

int g_Port = 5000;
size_t g_Clients = 100;
int g_Benchtime = 1;
std::atomic<size_t> g_Success(0);
std::atomic<size_t> g_Fail(0);
std::atomic<size_t> g_Bytes(0);

void worker(std::string path) {
  std::string http_request;
  http_request += "GET ";
  http_request += path;
  http_request +=
      " HTTP/1.1\r\n"
      "User-Agent: Web Benchmark\r\n"
      "Host: 127.0.0.1\r\n"
      "\r\n";

  /* create a socket */
  int sockfd = Socket(g_Port);
  if (sockfd < 0) {
    ++g_Fail;
    close(sockfd);
    return;
  }

  /* set the timer through SO_RCVTIMEO (without receiving msg, the socket will close automatically) */
  struct timeval tv;
  tv.tv_sec = g_Benchtime;
  tv.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

  // send http, we only write once, block if the send buffer is full
  ssize_t write_bytes = write(sockfd, http_request.c_str(), http_request.size());
  if (write_bytes < 0) {
    ++g_Fail;
    close(sockfd);
    return;
  }

  /* read until done */
  ssize_t total = 0;
  char buf[BUFFER_SIZE];
  while (true) {
    bzero(&buf, sizeof(buf));
    ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
    if (read_bytes < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      }
      // other case is error
      ++g_Fail;
      g_Bytes += total;
      close(sockfd);
      return;
    } else if (read_bytes == 0) {
      // server socket disconnected, which is not allowed
      ++g_Fail;
      g_Bytes += total;
      close(sockfd);
      return;
    } else {
      total += read_bytes;
    }
  }
  close(sockfd);
  ++g_Success;
  g_Bytes += total;
}

int main(int argc, char* argv[]) {
  /* opt parsing */
  int opt;
  std::string usage = "Usage: ";
  usage += argv[0];
  usage += " -p port -c clients -t time\n";
  int parseNTimes = 0;
  while ((opt = getopt(argc, argv, "p:c:t:")) != -1) {
    switch (opt) {
      case 'p':
        std::cout << "Port: " << optarg << std::endl;
        g_Port = atoi(optarg);
        break;
      case 'c':
        std::cout << "Clients: " << optarg << std::endl;
        g_Clients = atoi(optarg);
        break;
      case 't':
        std::cout << "Time: " << optarg << std::endl;
        g_Benchtime = atoi(optarg);
        break;
      default:
        std::cerr << usage;
        return 1;
    }
    ++parseNTimes;
  }
  if (parseNTimes < 3) {
    std::cerr << usage;
    return 1;
  }

  /* opt checking */
  if (g_Port < 1 || g_Port > 65535) {
    std::cerr << "Port must be between 1 and 65535\n";
    return 1;
  }
  if (g_Clients <= 0) {
    std::cerr << "Clients must be greater than 0\n";
    return 1;
  }
  if (g_Benchtime <= 0) {
    std::cerr << "Time must be greater than 0\n";
    return 1;
  }

  /* static file paths */
  std::vector<std::string> staticFiles;
  std::filesystem::path staticPath = "/home/wzy/code/cpp-server/static";
  namespace fs = std::filesystem;
  for (const auto& entry : fs::recursive_directory_iterator(staticPath)) {
    if (fs::is_regular_file(entry.status())) {
      staticFiles.emplace_back('/' + fs::relative(entry.path(), staticPath).string());
    }
  }

  /* test connection */
  int sockfd = Socket(g_Port);
  if (sockfd < 0) {
    std::cerr << "Socket connect error\n";
    return 1;
  }

  /* start timer */
  auto start = std::chrono::high_resolution_clock::now();

  /* create sub threads */
  std::vector<std::thread> threads;
  for (size_t i = 0; i < g_Clients; ++i) {
    threads.emplace_back(worker, staticFiles[i % staticFiles.size()]);
  }

  for (auto& t : threads) {
    t.join();
  }

  /* timer ends */
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

  /* print out final info */
  std::cout << "\n====================\n";
  std::cout << "processing speed=" << (g_Success + g_Fail) / (duration) << "req/s"
            << "\nRequest: "
            << "success=" << g_Success << ", fail=" << g_Fail << ", byte rate=" << (g_Bytes / duration) << " bytes/s\n";
}
