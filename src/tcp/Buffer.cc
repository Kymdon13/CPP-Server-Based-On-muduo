#include "Buffer.h"

#include <string.h>

#include <iostream>
#include <string>
#include <utility>

size_t Buffer::Size() const { return buffer_.size(); }

const char *Buffer::GetBuffer() const { return buffer_.c_str(); }
void Buffer::SetBuffer(const char *str) {
  std::string tmp(str);
  buffer_ = std::move(tmp);
}

void Buffer::Append(const char *str, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    if (str[i] == '\0') {
      break;
    }
    buffer_.push_back(str[i]);
  }
}
void Buffer::Clear() { buffer_.clear(); }
