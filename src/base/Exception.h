#pragma once

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

inline void errorif(bool condition, const char* errmsg) {
  if (condition) {
    std::string prefix = "errorif: ";
    std::string msg = prefix + errmsg;
    perror(msg.c_str());
    exit(EXIT_FAILURE);
  }
}

inline void warnif(bool condition, const char* wrnmsg) {
  if (condition) {
    std::string prefix = "warnif: ";
    std::string msg = prefix + wrnmsg;
    perror(msg.c_str());
  }
}

class Exception : public std::runtime_error {
 public:
  enum class ErrorCode : uint8_t { INVALID = 0, };
  Exception(ErrorCode code, const std::string& msg) : std::runtime_error(msg), code_(code) {}

  ErrorCode code() const noexcept { return code_; }

 private:
  ErrorCode code_;
};

class bigfile_error {
 public:
  explicit bigfile_error(const std::string& msg) : msg_(msg) {}
  const char* what() const noexcept { return msg_.c_str(); }
 private:
  std::string msg_;
};
