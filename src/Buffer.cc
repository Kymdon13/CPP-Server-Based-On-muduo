#include "Buffer.h"

#include <string.h>

#include <iostream>

const char *Buffer::GetBuffer() const { return buffer_.c_str(); }

size_t Buffer::Size() const { return buffer_.size(); }

void Buffer::Append(const char *str, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    if (str[i] == '\0') {
      break;
    }
    buffer_.push_back(str[i]);
  }
}

void Buffer::SetBuffer(const char *str) {
  std::string tmp(str);
  buffer_ = str;
}

void Buffer::Clear() { buffer_.clear(); }
