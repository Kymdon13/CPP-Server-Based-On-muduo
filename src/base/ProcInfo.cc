#include "ProcInfo.h"

#include <unistd.h>  // getpid, gethostname

pid_t ProcInfo::pid() { return ::getpid(); }

std::string ProcInfo::hostname() {
  char buf[256];
  if (::gethostname(buf, sizeof(buf)) == 0) {
    return std::string(buf);
  } else {
    return std::string("unknownhost");
  }
}
