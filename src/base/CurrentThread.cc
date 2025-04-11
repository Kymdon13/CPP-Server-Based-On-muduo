#include "CurrentThread.h"

namespace CurrentThread {

thread_local pid_t t_cachedTid = -1;
thread_local char t_formattedTid[32];
thread_local int t_formattedTidLength = 8;

}  // namespace CurrentThread
