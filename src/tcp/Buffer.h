#pragma once

#include <memory>
#include <string>

#include "base/cppserver-common.h"

class Buffer {
 private:
  std::string buffer_;

 public:
  DISABLE_COPYING_AND_MOVING(Buffer);
  Buffer() = default;
  ~Buffer() = default;

  size_t Size() const;

  const char *GetBuffer() const;
  void SetBuffer(const char *str);

  void Append(const char *str, size_t size);
  void Clear();
};
