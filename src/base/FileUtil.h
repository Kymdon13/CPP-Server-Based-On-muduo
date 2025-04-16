#include <cstdio>

#include <memory>
#include <string>
#include <vector>

#include "base/cppserver-common.h"
#include "tcp/Buffer.h"

namespace FileUtil {

class AppendFile {
 public:
  DISABLE_COPYING_AND_MOVING(AppendFile);
  explicit AppendFile(const std::string &path = "log/test.log");
  ~AppendFile();

  void append(const char *buf, size_t len);
  void flush();
  size_t writtenBytes() const { return written_bytes_; }

 private:
  FILE *fp_;
  char buffer_[64 * 1024];
  size_t written_bytes_;

  size_t write(const char *buf, size_t len);
};

class ReadFile {
 public:
  using BufferPtr = std::shared_ptr<Buffer>;

  DISABLE_COPYING_AND_MOVING(ReadFile);
  explicit ReadFile(const std::string &path, bool binary = false);
  ~ReadFile() {}

  BufferPtr data() { return data_; }

 private:
  BufferPtr data_;
};

}  // namespace FileUtil
