#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <thread>

#include "base/FileUtil.h"

std::atomic<bool> ready(false);

void server(int port) {
  /* create server */
  int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  // reuse the address
  int optval = 1;
  ::setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  bind(serv_fd, (struct sockaddr*)&addr, sizeof(addr));
  if (listen(serv_fd, 10) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  std::cout << "Server started" << std::endl;
  int clnt_fd = accept(serv_fd, NULL, NULL);
  std::cout << "Server accepted" << std::endl;

  char buffer[4096];
  while (true) {
    ssize_t read_bytes = read(clnt_fd, buffer, sizeof(buffer));
    if (read_bytes <= 0) break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait for some time
  close(clnt_fd);
  close(serv_fd);
  std::cout << "Server closed" << std::endl;
}

int main() {
  std::filesystem::path staticPath = "/home/wzy/code/cpp-server/static";
  FileLRU fileLRU(staticPath);

  int port = 8889;

  std::thread server_thread(server, port);

  int sockfd;
  struct sockaddr_in addr_clnt;
  bzero(&addr_clnt, sizeof(addr_clnt));
  addr_clnt.sin_family = AF_INET;
  addr_clnt.sin_port = htons(port);
  // BUG(wzy) do not use INADDR_LOOPBACK, will cause connection refused
  addr_clnt.sin_addr.s_addr = inet_addr("127.0.0.1");

  while (true) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << "Connecting to the local server on: " << port << std::endl;
    if (connect(sockfd, (struct sockaddr*)&addr_clnt, sizeof(addr_clnt)) == 0) {
      std::cout << "Client connected to the server on: " << port << std::endl;
      break;
    }
  }

  /**
   * FileLRU
   */

  /* static file paths */
  std::vector<std::string> staticFiles;
  namespace fs = std::filesystem;
  for (const auto& entry : fs::recursive_directory_iterator(staticPath)) {
    if (fs::is_regular_file(entry.status())) {
      staticFiles.emplace_back('/' + fs::relative(entry.path(), staticPath).string());
    }
  }

  auto start = std::chrono::high_resolution_clock::now();  // start timer
  /* run for ten thousand times */
  for (size_t i = 0; i < 10000; ++i) {
    auto file = fileLRU.getFile(staticFiles[i % staticFiles.size()]);
    write(sockfd, file->peek(), file->readableBytes());
  }
  auto end = std::chrono::high_resolution_clock::now();  // end timer
  auto duration_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "====================" << std::endl;
  std::cout << "FileLRU" << std::endl;
  std::cout << "Time taken: " << duration_milliseconds << " milliseconds" << std::endl;
  std::cout << "Time taken: " << duration_microseconds << " microseconds" << std::endl;

  /**
   * sendfile
   */

  /* static file paths */
  staticFiles.clear();
  for (const auto& entry : fs::recursive_directory_iterator(staticPath)) {
    if (fs::is_regular_file(entry.status())) {
      staticFiles.emplace_back(fs::relative(entry.path(), staticPath).string());
    }
  }

  start = std::chrono::high_resolution_clock::now();  // start timer
  /* run for ten thousand times */
  for (size_t i = 0; i < 10000; ++i) {
    int file_fd = open((staticPath / staticFiles[i % staticFiles.size()]).c_str(), O_RDONLY);
    if (file_fd < 0) {
      perror("open");
      exit(EXIT_FAILURE);
    }
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) < 0) {
      perror("fstat");
      exit(EXIT_FAILURE);
    }
    off_t offset = 0;
    size_t file_size = file_stat.st_size;
    sendfile(sockfd, file_fd, &offset, file_size);
  }
  end = std::chrono::high_resolution_clock::now();  // end timer
  duration_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "====================" << std::endl;
  std::cout << "sendfile" << std::endl;
  std::cout << "Time taken: " << duration_milliseconds << " milliseconds" << std::endl;
  std::cout << "Time taken: " << duration_microseconds << " microseconds" << std::endl;

  /**
   * ReadFile
   */
  start = std::chrono::high_resolution_clock::now();  // start timer
  /* run for ten thousand times */
  for (size_t i = 0; i < 10000; ++i) {
    ReadFile readFile(staticPath / staticFiles[i % staticFiles.size()]);
    readFile.read();
    auto file = readFile.data();
    write(sockfd, file->peek(), file->readableBytes());
  }
  end = std::chrono::high_resolution_clock::now();  // end timer
  duration_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "====================" << std::endl;
  std::cout << "ReadFile" << std::endl;
  std::cout << "Time taken: " << duration_milliseconds << " milliseconds" << std::endl;
  std::cout << "Time taken: " << duration_microseconds << " microseconds" << std::endl;
  std::cout << "====================" << std::endl;

  close(sockfd);
  // remember to wait for the server thread to finish, otherwise it will be terminated and be considered as a bug
  server_thread.join();
}