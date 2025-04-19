#pragma once

#include <string.h>

#include <string>

#include "base/common.h"

// 4KB
#define FIXED_BUFFER_SIZE 4096
// 4MB
#define FIXED_BUFFER_SIZE_BIG 4096 * 1024

template <size_t SIZE>
class FixedBuffer {
 private:
  char data_[SIZE];
  char* cur_ = data_;

  const char* end() const { return data_ + sizeof(data_); }

 public:
  DISABLE_COPYING_AND_MOVING(FixedBuffer);
  FixedBuffer() {}
  ~FixedBuffer() {}

  // append the BUF of length LEN to the buffer
  void append(const char* /*restrict*/ buf, size_t len) {
    if (static_cast<size_t>(avail()) > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { memset(data_, 0, sizeof(data_)); }
};

class LogStream {
 private:
  // redundant space to store extra info like '-', '+', 'e', '.', '\0', etc.
  static const int MAX_NUMERIC_SIZE = 48;
  FixedBuffer<FIXED_BUFFER_SIZE> buffer_;

  template <typename T>
  void formatInteger(T val);

  typedef LogStream self;

 public:
  typedef FixedBuffer<FIXED_BUFFER_SIZE> Buffer;

  DISABLE_COPYING_AND_MOVING(LogStream);
  LogStream() = default;
  ~LogStream() = default;

  // implemented in the LogStream.cc
  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);
  // floating point types
  self& operator<<(float);
  self& operator<<(double);
  // unknown type, convert to hex (0xab)
  self& operator<<(const void*);

  // string type
  self& operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }
  self& operator<<(const char* str) {
    if (str) {
      buffer_.append(str, strlen(str));
    } else {
      buffer_.append("(null)", 6);
    }
    return *this;
  }
  self& operator<<(const unsigned char* str) { return operator<<(reinterpret_cast<const char*>(str)); }
  self& operator<<(const std::string& str) {
    buffer_.append(str.c_str(), str.size());
    return *this;
  }
  self& operator<<(const FixedBuffer<FIXED_BUFFER_SIZE>& buf) {
    buffer_.append(buf.data(), buf.length());
    return *this;
  }

  // LogStream public interface
  void append(const char* buf, size_t len) { buffer_.append(buf, len); }
  const FixedBuffer<FIXED_BUFFER_SIZE>& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }
  void bzeroBuffer() { buffer_.bzero(); }
};

// a wrapper for snprintf
// class Fmt {
//  private:
//   char buf_[32];
//   int length_;

//  public:
//   template <typename T>
//   Fmt(const char *fmt, T val);

//   const char *data() const { return buf_; }
//   int length() const { return length_; }
// };

// inline LogStream &operator<<(LogStream &s, const Fmt &fmt) {
//   s.append(fmt.data(), fmt.length());
//   return s;
// }
