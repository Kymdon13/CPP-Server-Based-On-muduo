#pragma once
#include <sys/stat.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/common.h"
#include "log/Logger.h"
#include "tcp/Buffer.h"

class AppendFile {
 public:
  DISABLE_COPYING_AND_MOVING(AppendFile);
  explicit AppendFile(const std::string& path = "log/test.log");
  ~AppendFile();

  void append(const char* buf, size_t len);
  void flush();
  size_t writtenBytes() const { return written_bytes_; }

 private:
  FILE* fp_;
  char buffer_[64 * 1024];
  size_t written_bytes_;

  size_t write(const char* buf, size_t len);
};

class ReadFile {
 public:
  using BufferPtr = std::shared_ptr<Buffer>;

  DISABLE_COPYING_AND_MOVING(ReadFile);
  explicit ReadFile(const std::filesystem::path& path, bool binary = false)
      : path_(path), binary_(binary), data_(nullptr) {
    memset(&stat_, 0, sizeof(stat_));
  }
  ~ReadFile() {}

  void read();
  std::filesystem::path path() const { return path_; }
  BufferPtr& data() { return data_; }
  // XXX(wzy) do not use this slow io function
  /// @brief return the inode of the file
  /// @return 0 if stat() failed
  // struct stat stat();

 private:
  std::filesystem::path path_;
  struct stat stat_;
  bool binary_;
  BufferPtr data_;
};

class FileLRU {
 public:
  using BufferPtr = std::shared_ptr<Buffer>;
  /* node of the doubly-linked list */
  struct Node {
    std::string key__;
    BufferPtr buf__;
    Node* prev__;
    Node* next__;
    Node(const std::string& key, BufferPtr buf) : key__{key}, buf__{buf}, prev__(nullptr), next__(nullptr) {}
  };

 public:
  DISABLE_COPYING_AND_MOVING(FileLRU);
  FileLRU(const std::filesystem::path& path, size_t capacity = 512 * 1024 * 1024)
      : path_(path), capacity_(capacity), head_(new Node("", nullptr)), tail_(new Node("", nullptr)) {
    head_->next__ = tail_;
    tail_->prev__ = head_;
  }

  ~FileLRU() {
    Node* node = head_;
    while (node) {
      Node* next = node->next__;
      delete node;
      node = next;
    }
  }

  /// @brief
  /// @param request_path must start with '/'
  /// @return
  BufferPtr getFile(std::filesystem::path request_path);

 private:
  void push_back(Node* node);
  void push_front(Node* node);
  Node* remove(Node* node);

  BufferPtr get(const std::string& key);
  BufferPtr put(const std::string& key);

  std::filesystem::path path_;
  size_t size_ = 0;      // current file size in bytes
  size_t capacity_ = 0;  // max file size in bytes
  std::mutex mtx_;
  std::unordered_map<std::string, Node*> map_;
  Node* head_;  // points to least recently used
  Node* tail_;  // points to most recently used
};
