#include "LogStream.h"

#include <string.h>

#include <algorithm>
#include <string>
#include <type_traits>

#include "Grisu3.h"

// template <typename T>
// Fmt::Fmt(const char *fmt, T val) {
//   static_assert(std::is_arithmetic<T>::value, "T must be arithmetic type");
//   // length_ must be less than sizeof(buf_), otherwise it will write to other's memory
//   length_ = snprintf(buf_, sizeof(buf_), fmt, val);
// }

// // Explicit instantiations
// template Fmt::Fmt(const char *fmt, char);
// // integer types
// template Fmt::Fmt(const char *fmt, short);
// template Fmt::Fmt(const char *fmt, unsigned short);
// template Fmt::Fmt(const char *fmt, int);
// template Fmt::Fmt(const char *fmt, unsigned int);
// template Fmt::Fmt(const char *fmt, long);
// template Fmt::Fmt(const char *fmt, unsigned long);
// template Fmt::Fmt(const char *fmt, long long);
// template Fmt::Fmt(const char *fmt, unsigned long long);
// // floating point types
// template Fmt::Fmt(const char *fmt, float);
// template Fmt::Fmt(const char *fmt, double);

// Efficient Integer to String Conversions, by Matthew Wilson.
template <typename T>
size_t convertInt(char buf[], T value) {
  bool is_negative = value < 0;
  T i = is_negative ? -value : value;
  char* p = buf;
  do {
    int lsd = static_cast<int>(i % 10);  // lsd stands for 'least significant digit'
    i /= 10;
    *p++ = '0' + lsd;
  } while (i != 0);
  // add '-' if it's negative
  if (is_negative) {
    *p++ = '-';
  }
  // put an end to it
  *p = '\0';
  std::reverse(buf, p);
  return p - buf;
}
template <typename T>
void LogStream::formatInteger(T val) {
  if (buffer_.avail() >= MAX_NUMERIC_SIZE) {
    // directly write to buffer_ without using append()
    size_t len = convertInt(buffer_.current(), val);
    buffer_.add(len);
  }
}
/**
 * integer types
 */
LogStream& LogStream::operator<<(int v) {
  formatInteger(v);
  return *this;
}
LogStream& LogStream::operator<<(unsigned int v) {
  formatInteger(v);
  return *this;
}
LogStream& LogStream::operator<<(long v) {
  formatInteger(v);
  return *this;
}
LogStream& LogStream::operator<<(unsigned long v) {
  formatInteger(v);
  return *this;
}
LogStream& LogStream::operator<<(long long v) {
  formatInteger(v);
  return *this;
}
LogStream& LogStream::operator<<(unsigned long long v) {
  formatInteger(v);
  return *this;
}
LogStream& LogStream::operator<<(short v) {
  *this << static_cast<int>(v);
  return *this;
}
LogStream& LogStream::operator<<(unsigned short v) {
  *this << static_cast<unsigned int>(v);
  return *this;
}

/**
 * floating point types, use grisu3 to efficiently convert double to string
 */
LogStream& LogStream::operator<<(double v) {
  if (buffer_.avail() >= MAX_NUMERIC_SIZE) {
    // directly write to buffer_ without using append()
    size_t len = static_cast<size_t>(grisu3::dtoa_grisu3(v, buffer_.current()));
    buffer_.add(len);
  }
  return *this;
}
LogStream& LogStream::operator<<(float v) {
  *this << static_cast<double>(v);
  return *this;
};

const char digitsHex[] = "0123456789ABCDEF";
size_t convertHex(char buf[], uintptr_t value) {
  uintptr_t i = value;
  char* p = buf;
  do {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);
  // put an end to it
  *p = '\0';
  std::reverse(buf, p);  // reverse cuz we get the char starting from least significant bit (LSB)
  return p - buf;
}
/**
 * unknown type
 */
LogStream& LogStream::operator<<(const void* p) {
  uintptr_t v = reinterpret_cast<uintptr_t>(p);
  if (buffer_.avail() >= FIXED_BUFFER_SIZE) {
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = convertHex(buf + 2, v);
    buffer_.add(len + 2);
  }
  return *this;
}
