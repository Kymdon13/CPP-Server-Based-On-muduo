#pragma once

#include <memory>
#include <string>

#include "cppserver-common.h"

class Buffer {
 private:
  std::string buffer_;

 public:
  DISABLE_COPYING_AND_MOVING(Buffer);
  Buffer() = default;
  ~Buffer() = default;

  const char *GetBuffer() const;

  size_t Size() const;
  void Append(const char *str, size_t size);
  void SetBuffer(const char *str);
  void Clear();
};
