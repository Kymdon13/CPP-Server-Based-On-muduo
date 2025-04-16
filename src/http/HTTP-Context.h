#pragma once

#include <memory>
#include <string>

#define CR '\r'
#define LF '\n'

class HTTPRequest;

class HTTPContext {
 public:
  enum class HTTPRequestParseState : uint8_t {
    INVALID = 0,
    INVALID_METHOD,
    INVALID_URL,
    INVALID_PROTOCOL,
    INVALID_HEADER,
    INVALID_CRLF,
    COMPLETE,

    INIT,  // if in this state, then we have no history request need to remember, after parse complete we will reset the
           // state to INIT

    START,   // start parsing
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
    std::string last_req__;
    HTTPContext::HTTPRequestParseState parsingState__ = HTTPRequestParseState::INIT;
    long colon__;
  };

 private:
  int content_length_;
  HTTPRequestParseState state_;
  std::unique_ptr<HTTPRequest> http_request_;
  std::unique_ptr<ParsingSnapshot> snapshot_;

 public:
  HTTPContext();
  ~HTTPContext();

  void ResetState();
  bool IsComplete();
  HTTPRequest *GetHTTPRequest();
  HTTPRequestParseState ParseRequest(const char *begin, size_t size);
};
