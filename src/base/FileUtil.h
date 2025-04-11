#include <cstdio>

#include <string>

#include "base/cppserver-common.h"

namespace FileUtil {

class AppendFile {
 public:
  DISABLE_COPYING_AND_MOVING(AppendFile);
  explicit AppendFile(const std::string &path = "log/test.log");
  ~AppendFile();

  void append(const char *logline, size_t len);
  void flush();
  size_t writtenBytes() const { return written_bytes_; }

 private:
  FILE *fp_;
  char buffer_[64 * 1024];
  size_t written_bytes_;

  size_t write(const char *logline, size_t len);
};

}  // namespace FileUtil
