#pragma once

// Macros to disable copying and moving semantics
#define DISABLE_COPYING(cls) \
  cls(const cls &) = delete; \
  cls &operator=(const cls &) = delete;

#define DISABLE_MOVING(cls) \
  cls(cls &&) = delete;     \
  cls &operator=(cls &&) = delete;

#define DISABLE_COPYING_AND_MOVING(cls) \
  DISABLE_COPYING(cls);                 \
  DISABLE_MOVING(cls);

enum class RC {  // return code
  RC_UNDEFINED,
  RC_SUCCESS,
  RC_SOCKET_ERROR,
  RC_POLLER_ERROR,
  RC_CONNECTION_ERROR,
  RC_ACCEPTOR_ERROR,
  RC_UNIMPLEMENTED
};

// for event in Channel
typedef uint32_t event_t;
