#pragma once

#include <memory>
#include <string>

#define CR '\r'
#define LF '\n'

class HTTPRequest;

enum class HTTPRequestParseState : unsigned char {
  // TODO(wzy) need to implement different types of invalid state
  INVALID,
  INVALID_METHOD,
  INVALID_URL,
  INVALID_VERSION,
  INVALID_HEADER,

  START,   // start parsing
  METHOD,  // request method

  BEFORE_URL,  // before '/'
  URL,

  BEFORE_URL_PARAM_KEY,  // check if there is space, CR or LF after '?'
  URL_PARAM_KEY,
  BEFORE_URL_PARAM_VALUE,  // check if there is space, CR or LF after '='
  URL_PARAM_VALUE,

  BEFORE_PROTOCOL,  // used to skip space before the protocol
  PROTOCOL,

  BEFORE_VERSION,
  VERSION,

  // TODO(wzy) not robust when parsing headers
  HEADER,
  HEADER_KEY,
  HEADER_VALUE,

  ENCOUNTER_CR,  // encounter a CR
  CR_LF,         // if next char is CR then it's BODY which follows, otherwise it's header
  CR_LF_CR,      // encounter CR after CR_LF

  BODY,

  COMPLETE,
};

class HTTPContext {
 private:
  std::unique_ptr<HTTPRequest> http_request_;
  HTTPRequestParseState state_;
  int content_length_;

 public:
  HTTPContext();
  ~HTTPContext();

  void ResetState();
  bool IsComplete();
  HTTPRequest *GetHTTPRequest();
  HTTPRequestParseState ParseRequest(const char *begin, ssize_t size);
};
