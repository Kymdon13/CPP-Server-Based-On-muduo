#include <algorithm>
#include <iostream>
#include <string>

namespace utils {

std::string &toLower(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
  return str;
}

}  // namespace utils
