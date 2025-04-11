#include "FileUtil.h"

#include <filesystem>

FileUtil::AppendFile::AppendFile(const std::string &path) : fp_(nullptr), written_bytes_(0) {
  std::filesystem::path filepath(path);
  // create dir
  std::filesystem::create_directories(filepath.parent_path());
  fp_ = ::fopen(filepath.string().c_str(), "ae");
  // set buffer for reducing io
  ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

FileUtil::AppendFile::~AppendFile() {
  // will flush the buffer before close
  ::fclose(fp_);
}

void FileUtil::AppendFile::append(const char *logline, size_t len) {
  size_t written = 0;
  written = write(logline + written, len);
  while (written != len) {
    written += write(logline + written, len - written);
  }
  written_bytes_ += written;
}

void FileUtil::AppendFile::flush() { ::fflush(fp_); }

size_t FileUtil::AppendFile::write(const char *logline, size_t len) {
  // ::fwrite_unlocked(data_ptr, size_of_one_element_in_byte, number_of_elements, file_ptr) returns the number of
  // elements written (negative if error) we use unlocked version to speed up, cuz there is only one background thread
  // doing the logging
  return ::fwrite_unlocked(logline, 1, len, fp_);
}
