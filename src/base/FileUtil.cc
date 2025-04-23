#include "FileUtil.h"

#include <fstream>
#include <stdexcept>

AppendFile::AppendFile(const std::string& path) : fp_(nullptr), written_bytes_(0) {
  std::filesystem::path filepath(path);
  std::filesystem::create_directories(filepath.parent_path());
  fp_ = ::fopen(filepath.string().c_str(), "ae");
  // set buffer to reduce io
  ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

AppendFile::~AppendFile() {
  // will flush the buffer before close
  ::fclose(fp_);
}

void AppendFile::append(const char* buf, size_t len) {
  size_t written = 0;
  written = write(buf + written, len);
  while (written != len) {
    written += write(buf + written, len - written);
  }
  written_bytes_ += written;
}

void AppendFile::flush() { ::fflush(fp_); }

size_t AppendFile::write(const char* buf, size_t len) {
  // ::fwrite_unlocked(data_ptr, size_of_one_element_in_byte, number_of_elements, file_ptr) returns the number of
  // elements written (negative if error) we use unlocked version to speed up, cuz there is only one background thread
  // doing the logging
  return ::fwrite_unlocked(buf, 1, len, fp_);
}

void ReadFile::read() {
  std::ifstream ifs;
  if (binary_) {
    ifs.open(path_, std::ios::binary);
  } else {
    ifs.open(path_);
  }
  if (!ifs.is_open()) {
    throw std::filesystem::filesystem_error("FileUtil::ReadFile::read(), failed to open file: " + path_.string(),
                                            std::make_error_code(std::errc::no_such_file_or_directory));
  }
  // get file size
  size_t fileSize = std::filesystem::file_size(path_);
  // preallocate string to hold the file content
  data_ = std::make_shared<Buffer>(fileSize, 0);

  // read file into Buffer
  const std::streamsize bufferSize = 64 * 1024;  // read bufferSize bytes at a time
  std::streamsize bytesRead = 0;
  while (ifs.read(data_->beginWrite() + bytesRead, bufferSize)) {
    bytesRead += ifs.gcount();
  }
  bytesRead += ifs.gcount();  // read the remaining bytes
  data_->hasWritten(static_cast<size_t>(bytesRead));
}
struct stat ReadFile::stat() {
  if (stat_.st_ino != 0) {
    return stat_;
  }
  struct stat st;
  if (::stat(path_.c_str(), &st) != -1) {
    stat_ = st;
  } else {
    int err = errno;
    if (err == ENOENT) {
      throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
                              "ReadFile::stat(), " + path_.string());
    } else if (err == EACCES) {
      throw std::system_error(std::make_error_code(std::errc::permission_denied),
                              "ReadFile::stat(), " + path_.string());
    } else {
      throw std::system_error(std::make_error_code(std::errc::io_error), "ReadFile::stat(), " + path_.string());
    }
  }
  return st;
}

FileLRU::BufferPtr FileLRU::getFile(const std::filesystem::path& rel_or_abs_path) {
  std::unique_lock<std::mutex> lock(mtx_);
  try {
    ReadFile file(path_ / rel_or_abs_path);  // if rel_or_abs_path is abs path, operator / will return the abs path
    ino_t key = file.stat().st_ino;
    time_t mtime = 0;

    BufferPtr value = get(key, &mtime);
    if (value) {
      // if modified time is greater than the file's mtime, update the file
      // caution: you must return file.data() after put(file), cuz value will be out-of-date after put(file)
      if (mtime > file.stat().st_mtim.tv_sec) {
        put(file);
        return file.data();
      }
      return value;
    } else {
      put(file);
      return file.data();
    }
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << e.what();
  } catch (const std::system_error& e) {
    LOG_ERROR << e.what();
  } catch (const std::runtime_error& e) {
    LOG_ERROR << e.what();
  }
  return nullptr;
}

FileLRU::BufferPtr FileLRU::get(ino_t key, time_t* mtime) {
  if (map_.count(key)) {
    if (mtime) {
      *mtime = map_[key]->mtime__;
    }
    // move the node to the tail
    push_back(remove(map_[key]));
    return map_[key]->buf__;
  }
  return nullptr;
}

void FileLRU::put(ReadFile& file) {
  // check if the file is oversized
  ino_t key = file.stat().st_ino;
  file.read();
  BufferPtr value = file.data();
  if (value->readableBytes() > capacity_) {
    throw std::runtime_error("FileUtil::FileLRU::put(), file size exceeds capacity");
  }

  if (map_.count(key)) {  // if put existed key with different value
    // update size_
    size_ -= map_[key]->buf__->readableBytes();
    size_ += value->readableBytes();

    map_[key]->buf__ = value;
    push_back(remove(map_[key]));
  } else {
    // update size_
    size_ += value->readableBytes();

    Node* tmp = new Node{key, file.stat().st_mtim.tv_sec, value};

    if (size_ > capacity_) {  // if new size exceeds capacity
      // remove the least recently used node until the size is less than capacity
      do {
        map_.erase(head_->next__->key__);
        Node* tmp = remove(head_->next__);
        size_ -= tmp->buf__->readableBytes();
        delete tmp;
      } while (size_ > capacity_);
    }
    map_[key] = tmp;
    push_back(tmp);
  }
}

void FileLRU::push_back(Node* node) {
  node->prev__ = tail_->prev__;
  node->next__ = tail_;
  tail_->prev__->next__ = node;
  tail_->prev__ = node;
}
void FileLRU::push_front(Node* node) {
  node->prev__ = head_;
  node->next__ = head_->next__;
  head_->next__->prev__ = node;
  head_->next__ = node;
}
FileLRU::Node* FileLRU::remove(Node* node) {
  node->prev__->next__ = node->next__;
  node->next__->prev__ = node->prev__;
  node->prev__ = nullptr;
  node->next__ = nullptr;
  return node;
}
