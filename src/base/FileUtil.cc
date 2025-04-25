#include "FileUtil.h"

#include <fstream>
#include <list>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;

AppendFile::AppendFile(const std::string& path) : fp_(nullptr), written_bytes_(0) {
  fs::path filepath(path);
  fs::create_directories(filepath.parent_path());
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

bool ReadFile::is_text_file() const {
  static const std::set<std::string> text_extensions = {// HTML/CSS/JS
                                                        ".js", ".html", ".htm", ".css",
                                                        // config or data
                                                        ".json", ".env", ".xml", ".csv", ".txt", ".md", ".pug", ".ts",
                                                        ".jsx", ".tsx", ".yml", ".yaml",
                                                        // code
                                                        ".c", ".cc", ".cpp", ".h", ".hpp", ".java", ".py", ".sh"};
  std::string ext = path_.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return text_extensions.count(ext) > 0;
};

void ReadFile::read() {
  std::ifstream ifs;
  if (is_text_file()) {
    ifs.open(path_);
  } else {
    ifs.open(path_, std::ios::binary);
  }
  if (!ifs.is_open()) {
    throw fs::filesystem_error("FileUtil::ReadFile::read(), failed to open file: " + path_.string(),
                               std::make_error_code(std::errc::no_such_file_or_directory));
  }
  // get file size
  size_t fileSize = fs::file_size(path_);
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

// XXX(wzy) ::stat() is slow, do not use it
// struct stat ReadFile::stat() {
//   if (stat_.st_ino != 0) {
//     return stat_;
//   }
//   struct stat st;
//   if (::stat(path_.c_str(), &st) != -1) {
//     stat_ = st;
//   } else {
//     int err = errno;
//     if (err == ENOENT) {
//       throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
//                               "ReadFile::stat(), " + path_.string());
//     } else if (err == EACCES) {
//       throw std::system_error(std::make_error_code(std::errc::permission_denied),
//                               "ReadFile::stat(), " + path_.string());
//     } else {
//       throw std::system_error(std::make_error_code(std::errc::io_error), "ReadFile::stat(), " + path_.string());
//     }
//   }
//   return st;
// }

FileLRU::BufferPtr FileLRU::getFile(fs::path request_path) {
  try {
    BufferPtr value = get(request_path.string());
    if (value) {
      return value;
    } else {
      return put(request_path.string());
    }
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << e.what();
  } catch (const std::system_error& e) {
    LOG_ERROR << e.what();
  } catch (const std::runtime_error& e) {
    LOG_ERROR << e.what();
  }
  return nullptr;
}

FileLRU::BufferPtr FileLRU::get(const std::string& key) {
  std::shared_lock<std::shared_mutex> lock(rw_mtx_);
  if (map_.count(key)) {
    // move the node to the tail
    push_back(remove(map_[key]));
    return map_[key]->buf__;
  }
  return nullptr;
}

FileLRU::BufferPtr FileLRU::put(const std::string& key) {
  std::unique_lock<std::shared_mutex> lock(rw_mtx_);
  fs::path full_path;
  if (key[0] == '/') {
    full_path = path_ / fs::path(key).relative_path();
  } else {
    full_path = path_ / key;
  }

  // check existence
  if (!fs::exists(full_path)) {
    throw fs::filesystem_error("FileUtil::FileLRU::put(), file not exist: " + full_path.string(),
                               std::make_error_code(std::errc::no_such_file_or_directory));
  }

  // read the file
  ReadFile file(full_path);
  file.read();
  BufferPtr value = file.data();

  // check if the file is oversized
  if (value->readableBytes() > capacity_) {
    throw std::runtime_error("FileUtil::FileLRU::put(), file size exceeds capacity");
  }

  if (map_.count(key)) {                         // if insert existed key with different value
    size_ -= map_[key]->buf__->readableBytes();  // update size_
    size_ += value->readableBytes();             // update size_

    map_[key]->buf__ = value;
    push_back(remove(map_[key]));
  } else {
    size_ += value->readableBytes();  // update size_

    Node* tmp = new Node{key, value};

    // if the cache is full, remove the least recently used node
    if (size_ > capacity_) {
      // keep removing the least recently used node until the size is less than capacity
      do {
        map_.erase(head_->next__->key__);
        Node* tmp = remove(head_->next__);
        size_ -= tmp->buf__->readableBytes();  // update size_
        delete tmp;
      } while (size_ > capacity_);
    }

    // insert the new node to the tail
    map_[key] = tmp;
    push_back(tmp);
  }
  return value;
}

/**
 * for inotify usage
 */
void FileLRU::addFile(const fs::path& path) {
  try {
    // get path relative to static path
    fs::path relative_path = fs::relative(path, path_);
    std::string key = '/' + relative_path.string();  // make sure the key starts with '/'
    put(key);
  } catch (const fs::filesystem_error& e) {
    LOG_ERROR << e.what();
  } catch (const std::system_error& e) {
    LOG_ERROR << e.what();
  } catch (const std::runtime_error& e) {
    LOG_ERROR << e.what();
  }
}
void FileLRU::delFile(const std::filesystem::path& path) {
  std::unique_lock<std::shared_mutex> lock(rw_mtx_);
  fs::path relative_path = fs::relative(path, path_);
  std::string key = '/' + relative_path.string();  // make sure the key starts with '/'
  if (map_.count(key)) {
    Node* node = map_[key];
    size_ -= node->buf__->readableBytes();  // update size_
    delete remove(node);
    map_.erase(key);
  }
}
void FileLRU::delDir(const std::filesystem::path& path) {
  std::unique_lock<std::shared_mutex> lock(rw_mtx_);
  fs::path relative_path = fs::relative(path, path_);
  std::string key = '/' + relative_path.string();  // make sure the key starts with '/'
  for (auto it = map_.begin(); it != map_.end();) {
    if (it->first.find(key) == 0) {  // if the key starts with the path
      Node* node = it->second;
      size_ -= node->buf__->readableBytes();  // update size_
      delete remove(node);
      it = map_.erase(it);
    } else {
      ++it;
    }
  }
}
void FileLRU::moveFile(const std::filesystem::path& old_path, const std::filesystem::path& new_path) {
  std::unique_lock<std::shared_mutex> lock(rw_mtx_);
  std::string old_key = '/' + fs::relative(old_path, path_).string();  // make sure the key starts with '/'
  std::string new_key = '/' + fs::relative(new_path, path_).string();  // make sure the key starts with '/'
  if (map_.count(old_key)) {
    Node* node = map_[old_key];
    map_.erase(old_key);
    map_[new_key] = node;
  }
}
void FileLRU::moveDir(const std::filesystem::path& old_path, const std::filesystem::path& new_path) {
  std::unique_lock<std::shared_mutex> lock(rw_mtx_);
  std::string old_key = '/' + fs::relative(old_path, path_).string();  // make sure the key starts with '/'
  std::string new_key = '/' + fs::relative(new_path, path_).string();  // make sure the key starts with '/'
  std::list<std::pair<std::string, Node*>> adding_list;
  for (auto it = map_.begin(); it != map_.end();) {
    if (it->first.find(old_key) == 0) {  // if the key starts with the path
      adding_list.emplace_back(std::make_pair(new_path / old_path.filename(), it->second));
      it = map_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto& it : adding_list) {
    map_[it.first] = it.second;
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
