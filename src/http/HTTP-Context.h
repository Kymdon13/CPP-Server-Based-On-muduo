#pragma once

#include <memory>
#include <string>

#define CR '\r'
#define LF '\n'

class Buffer;
class HTTPRequest;

class HTTPContext {
 public:
  enum class ParseState : uint8_t {
    INVALID = 0,
    INVALID_METHOD,
    INVALID_URL,
    INVALID_PROTOCOL,
    INVALID_HEADER,
    INVALID_CRLF,
    COMPLETE,

    START,   // start parsing, after parse complete we will reset the state to START
    METHOD,  // request method

    BEFORE_URL,  // report INVALID_URL if not start with '/'
    URL,

    BEFORE_URL_PARAM_KEY,  // make sure there is at least one char after '?'
    URL_PARAM_KEY,
    BEFORE_URL_PARAM_VALUE,  // make sure there is at least one char after '='
    URL_PARAM_VALUE,

    BEFORE_PROTOCOL,  // report INVALID_PROTOCOL if not start with uppercase
    PROTOCOL,

    BEFORE_VERSION,  // make sure there is at least one char after 'HTTP/'
    VERSION,

    HEADER,
    HEADER_KEY,
    BEFORE_HEADER_VALUE,
    HEADER_VALUE,

    ENCOUNTER_CR,  // encounter a CR
    CR_LF,         // if next char is CR then it's BODY which follows, otherwise it's header
    CR_LF_CR,      // encounter CR after CR_LF

    BODY
  };

  struct ParsingSnapshot {
    // TODO(wzy) replace lastRequest__ with vector may be more efficient?
    std::string lastRequest__;
    HTTPContext::ParseState parseState__ = ParseState::START;
    long colon__;
  };

 private:
  size_t contentLength_;
  ParseState state_;
  std::unique_ptr<HTTPRequest> request_;
  std::unique_ptr<ParsingSnapshot> snapshot_;

 public:
  HTTPContext();
  ~HTTPContext();

  void reset();
  bool isComplete();
  HTTPRequest getRequest();
  ParseState parseRequest(Buffer* buffer);
};
