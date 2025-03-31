#include "include/CurrentThread.h"

namespace CurrentThread {

thread_local pid_t t_cachedTid = -1;

}  // namespace CurrentThread
