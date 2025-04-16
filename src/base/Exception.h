#pragma once

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

inline void errorif(bool condition, const char *errmsg) {
  if (condition) {
    std::string prefix = "errorif: ";
    std::string msg = prefix + errmsg;
    perror(msg.c_str());
    exit(EXIT_FAILURE);
  }
}

inline void warnif(bool condition, const char *wrnmsg) {
  if (condition) {
    std::string prefix = "warnif: ";
    std::string msg = prefix + wrnmsg;
    perror(msg.c_str());
  }
}

enum class ExceptionType { INVALID = 0, INVALID_SOCKET };

class Exception : public std::runtime_error {
 public:
  explicit Exception(const std::string &msg) : std::runtime_error(msg), type_(ExceptionType::INVALID) {
    std::string exception_msg = "Message :: " + msg + '\n';
    std::cerr << exception_msg;
  }

  Exception(ExceptionType type, const std::string &msg) : std::runtime_error(msg), type_(type) {
    std::string exception_msg = "Exception Type :: " + ExceptionTypeToString(type_) + "\nMessage :: " + msg + '\n';
    std::cerr << exception_msg;
  }

  static std::string ExceptionTypeToString(ExceptionType type) {
    switch (type) {
      case ExceptionType::INVALID:
        return "Invalid";
        break;
      case ExceptionType::INVALID_SOCKET:
        return "Invalid Socket";
        break;
      default:
        return "Unknown";
        break;
    }
  }

 private:
  ExceptionType type_;
};
