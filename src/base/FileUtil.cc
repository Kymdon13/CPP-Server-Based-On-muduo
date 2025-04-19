#include "FileUtil.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

FileUtil::AppendFile::AppendFile(const std::string& path) : fp_(nullptr), written_bytes_(0) {
  std::filesystem::path filepath(path);
  std::filesystem::create_directories(filepath.parent_path());
  fp_ = ::fopen(filepath.string().c_str(), "ae");
  // set buffer to reduce io
  ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

FileUtil::AppendFile::~AppendFile() {
  // will flush the buffer before close
  ::fclose(fp_);
}

void FileUtil::AppendFile::append(const char* buf, size_t len) {
  size_t written = 0;
  written = write(buf + written, len);
  while (written != len) {
    written += write(buf + written, len - written);
  }
  written_bytes_ += written;
}

void FileUtil::AppendFile::flush() { ::fflush(fp_); }

size_t FileUtil::AppendFile::write(const char* buf, size_t len) {
  // ::fwrite_unlocked(data_ptr, size_of_one_element_in_byte, number_of_elements, file_ptr) returns the number of
  // elements written (negative if error) we use unlocked version to speed up, cuz there is only one background thread
  // doing the logging
  return ::fwrite_unlocked(buf, 1, len, fp_);
}

FileUtil::ReadFile::ReadFile(const std::string& path, bool binary) {
  std::ifstream ifs;
  if (binary) {
    ifs.open(path, std::ios::binary);
  } else {
    ifs.open(path);
  }
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open file: " + path);
  }

  // get file size
  size_t fileSize = std::filesystem::file_size(path);
  // preallocate string to hold the file content
  data_ = std::make_shared<Buffer>(fileSize);

  const std::streamsize bufferSize = 64 * 1024;  // read bufferSize bytes at a time
  std::streamsize bytesRead = 0;
  while (ifs.read(data_->beginWrite() + bytesRead, bufferSize)) {
    bytesRead += ifs.gcount();
  }
  bytesRead += ifs.gcount();  // read the remaining bytes
  data_->hasWritten(static_cast<size_t>(bytesRead));
}
