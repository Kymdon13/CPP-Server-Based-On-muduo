#pragma once

#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace CurrentThread {

extern thread_local pid_t t_cachedTid;
extern thread_local char t_formattedTid[32];
extern thread_local int t_formattedTidLength;

inline int gettid() {
  if (unlikely(t_cachedTid == -1)) {
    t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
    t_formattedTidLength = snprintf(t_formattedTid, sizeof(t_formattedTid), "[%5d]", t_cachedTid);
  }
  return t_cachedTid;
}

inline const char *tidString() { return t_formattedTid; }
inline int tidStringLength() { return t_formattedTidLength; }

}  // namespace CurrentThread
