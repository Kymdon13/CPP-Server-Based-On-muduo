#include "HTTP-Context.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

#include "HTTP-Request.h"
#include "log/Logger.h"

HTTPContext::HTTPContext() : state_(HTTPRequestParseState::START), content_length_(0) {
  http_request_ = std::make_unique<HTTPRequest>();
}

HTTPContext::~HTTPContext(){};

void HTTPContext::ResetState() { state_ = HTTPRequestParseState::START; }

bool HTTPContext::IsComplete() { return state_ == HTTPRequestParseState::COMPLETE; }

HTTPRequest *HTTPContext::GetHTTPRequest() { return http_request_.get(); }

bool isInvalidURLChar(const char &c) {
  // control characters
  if (c >= '\x00' && c <= '\x1F') return true;
  if (c == '\x7F') return true;
  // reserved characters
  switch (c) {
    case ' ':
    case '!':
    case '#':
    case '$':
    case '&':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '/':
    case ':':
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '@':
    case '[':
    case '\'':
    case '\\':
    case '\n':
    case '\r':
    case ']':
    case '^':
    case '`':
    case '|':
      return true;
    default:
      return false;
  }
}
bool isInvalidHeaderKeyChar(const char &c) {
  // control characters
  if (c >= '\x00' && c <= '\x1F') return true;
  if (c == '\x7F') return true;
  // special characters
  switch (c) {
    // case ':':
    case ' ':
    case ',':
    case ';':
    case '<':
    case '=':
    case '>':
    case '@':
    case '\\':
    case '^':
    case '`':
    case '|':
      return true;
    default:
      return false;
  }
}

HTTPContext::HTTPRequestParseState HTTPContext::ParseRequest(const char *begin, size_t size,
                                                             parsingSnapshot *snapshot) {
  const char *start, *cur, *end, *colon;
  std::string combined_request;
  if (snapshot && snapshot->parsingState__ <= HTTPRequestParseState::COMPLETE) {  // not completed
    combined_request = snapshot->last_req__ + std::string(begin, size);
    start = combined_request.c_str();           // point to left border
    cur = start + snapshot->last_req__.size();  // point to beginning of the next request
    end = start + combined_request.size();
    colon = start + snapshot->colon__;  // restore the position of colon
  } else {
    start = begin;  // point to left border
    cur = start;    // point to right border
    end = start + size;
    colon = start;  // store the position of ':' if encounter 'url:params' or 'headerkey:headervalue'
  }
  while (state_ > HTTPRequestParseState::COMPLETE && cur < end) {
    char c = *cur;  // get current char
    switch (state_) {
      case HTTPRequestParseState::START: {
        if (isblank(c) || c == CR || c == LF) {  // skip with space, CR or LF
          break;
        }
        if (isupper(c)) {  // uppercase may be METHOD
          state_ = HTTPRequestParseState::METHOD;
          start = cur;
        } else {
          state_ = HTTPRequestParseState::INVALID_METHOD;  // method must be uppercase
        }
        break;
      }
      case HTTPRequestParseState::METHOD: {
        if (isupper(c)) {  // we are still in the METHOD
          break;
        }
        if (isblank(c)) {  // METHOD done, go to URL
          state_ = HTTPRequestParseState::BEFORE_URL;
          http_request_->SetMethod(std::string(start, cur));  // string(begin, end) is [begin, end)
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID_METHOD;  // method must be uppercase
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_URL: {
        if (c == '/') {
          state_ = HTTPRequestParseState::URL;
        } else {
          state_ = HTTPRequestParseState::INVALID_URL;  // URL must start with '/'
        }
        break;
      }
      case HTTPRequestParseState::URL: {
        if (c == '?') {  // encounter request params
          state_ = HTTPRequestParseState::BEFORE_URL_PARAM_KEY;
          http_request_->SetUrl(std::string(start, cur));  // string(begin, end) is [begin, end)
          start = cur + 1;
        } else if (isblank(c)) {  // done request params
          state_ = HTTPRequestParseState::BEFORE_PROTOCOL;
          http_request_->SetUrl(std::string(start, cur));  // string(begin, end) is [begin, end)
          start = cur + 1;
        } else if (isInvalidURLChar(c)) {
          state_ = HTTPRequestParseState::INVALID_URL;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_URL_PARAM_KEY: {
        if (isInvalidURLChar(c)) {  // after '?' or '&'
          state_ = HTTPRequestParseState::INVALID_URL;
        } else {
          state_ = HTTPRequestParseState::URL_PARAM_KEY;
        }
        break;
      }
      case HTTPRequestParseState::URL_PARAM_KEY: {
        if (c == '=') {  // done param key, go to param value
          state_ = HTTPRequestParseState::BEFORE_URL_PARAM_VALUE;
          colon = cur;
        } else if (isInvalidURLChar(c)) {
          state_ = HTTPRequestParseState::INVALID_URL;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_URL_PARAM_VALUE: {
        if (isInvalidURLChar(c)) {  // after '='
          state_ = HTTPRequestParseState::INVALID_URL;
        } else {
          state_ = HTTPRequestParseState::URL_PARAM_VALUE;
        }
        break;
      }
      case HTTPRequestParseState::URL_PARAM_VALUE: {
        if (c == '&') {  // encounter next param
          state_ = HTTPRequestParseState::BEFORE_URL_PARAM_KEY;
          http_request_->SetRequestParams(std::string(start, colon), std::string(colon + 1, cur));
          start = cur + 1;
        } else if (isblank(c)) {  // done request params
          state_ = HTTPRequestParseState::BEFORE_PROTOCOL;
          http_request_->SetRequestParams(std::string(start, colon), std::string(colon + 1, cur));
          start = cur + 1;
        } else if (isInvalidURLChar(c)) {
          state_ = HTTPRequestParseState::INVALID_URL;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_PROTOCOL: {
        if (isupper(c)) {  // uppercase may be PROTOCOL
          state_ = HTTPRequestParseState::PROTOCOL;
          start = cur;
        } else {
          state_ = HTTPRequestParseState::INVALID_PROTOCOL;  // PROTOCOL must be uppercase
        }
        break;
      }
      case HTTPRequestParseState::PROTOCOL: {
        if (isupper(c)) {  // we are still in the PROTOCOL
          break;
        }
        if (c == '/') {  // done PROTOCOL, go to VERSION
          state_ = HTTPRequestParseState::BEFORE_VERSION;
          http_request_->SetProtocol(std::string(start, cur));
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID_PROTOCOL;  // other cases is invalid
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_VERSION: {
        if (isdigit(c)) {
          state_ = HTTPRequestParseState::VERSION;
        } else {
          state_ = HTTPRequestParseState::INVALID_PROTOCOL;  // VERSION must has at least 1 digit
        }
        break;
      }
      case HTTPRequestParseState::VERSION: {
        if (c == CR) {  // done version
          state_ = HTTPRequestParseState::ENCOUNTER_CR;
          http_request_->SetVersion(std::string(start, cur));
          start = cur + 1;
        } else if (isdigit(c) || c == '.') {  // only allow digit and '.' in version
          break;
        } else {
          state_ = HTTPRequestParseState::INVALID_PROTOCOL;
        }
        break;
      }
      case HTTPRequestParseState::ENCOUNTER_CR: {
        if (c == LF) {  // '\n' after '\r', means this line is over
          state_ = HTTPRequestParseState::CR_LF;
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID_CRLF;  // CR not followed by LF
        }
        break;
      }
      case HTTPRequestParseState::CR_LF: {
        if (c == CR) {  // encounter blank line
          state_ = HTTPRequestParseState::CR_LF_CR;
        } else if (isInvalidHeaderKeyChar(c)) {  // if not CR, must be header key
          state_ = HTTPRequestParseState::INVALID_HEADER;
        } else {
          state_ = HTTPRequestParseState::HEADER_KEY;
        }
        break;
      }
      case HTTPRequestParseState::HEADER_KEY: {
        if (c == ':') {
          state_ = HTTPRequestParseState::BEFORE_HEADER_VALUE;
          colon = cur;
        } else if (isInvalidHeaderKeyChar(c)) {  // space not allowed in header key
          state_ = HTTPRequestParseState::INVALID_HEADER;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_HEADER_VALUE: {
        if (!isblank(c)) {
          state_ = HTTPRequestParseState::INVALID_HEADER;  // must be space between ':' and header value
        } else {
          state_ = HTTPRequestParseState::HEADER_VALUE;
        }
        break;
      }
      case HTTPRequestParseState::HEADER_VALUE: {
        if (c == CR) {
          state_ = HTTPRequestParseState::ENCOUNTER_CR;
          http_request_->AddHeader(
              std::string(start, colon),
              std::string(colon + 2, cur));  // colon+2 is because there is a space between ':' and the header value
          start = cur + 1;
        }
        // we do not inspect the header value here, cuz the it varies a lot
        break;
      }
      case HTTPRequestParseState::CR_LF_CR: {
        if (c == LF) {
          if (http_request_->GetHeaders().count("Content-Length")) {  // check if there is field "Content-Length"
            if (content_length_ = atoi(http_request_->GetHeaderByKey("Content-Length").c_str()); content_length_ > 0) {
              state_ = HTTPRequestParseState::BODY;
            } else {
              state_ = HTTPRequestParseState::COMPLETE;
            }
          } else {
            if (cur - begin + 1 < static_cast<long>(size)) {  // if LF is the last char, then cur-begin = size-1
              state_ = HTTPRequestParseState::BODY;
            } else {
              state_ = HTTPRequestParseState::COMPLETE;
            }
          }
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID_CRLF;
        }
        break;
      }
      case HTTPRequestParseState::BODY: {
        state_ = HTTPRequestParseState::COMPLETE;
        // if "Content-Length" is presented, use it as body length
        size_t bodylength;
        if (content_length_ != 0) {
          bodylength = std::min(static_cast<unsigned long>(content_length_), (size - (cur - begin)));
        } else {
          bodylength = size - (cur - begin);
        }
        http_request_->SetBody(std::string(start, start + bodylength));
        break;
      }
      default:
        state_ = HTTPRequestParseState::INVALID;
        break;
    }
    ++cur;
  }
  switch (state_) {
    case HTTPRequestParseState::COMPLETE:
      return HTTPRequestParseState::COMPLETE;
      break;
    case HTTPRequestParseState::INVALID:
      return HTTPRequestParseState::INVALID;
      break;
    case HTTPRequestParseState::INVALID_METHOD:
      return HTTPRequestParseState::INVALID_METHOD;
      break;
    case HTTPRequestParseState::INVALID_URL:
      return HTTPRequestParseState::INVALID_URL;
      break;
    case HTTPRequestParseState::INVALID_PROTOCOL:
      return HTTPRequestParseState::INVALID_PROTOCOL;
      break;
    case HTTPRequestParseState::INVALID_HEADER:
      return HTTPRequestParseState::INVALID_HEADER;
      break;
    case HTTPRequestParseState::INVALID_CRLF:
      return HTTPRequestParseState::INVALID_CRLF;
      break;
    default:
      // if the state is not complete, then we need to save the state
      if (snapshot) {
        snapshot->parsingState__ = state_;
        snapshot->last_req__ = std::string(start, cur);
        LOG_TRACE << "HTTPContext::ParseRequest: unfinished, last request: \"" << snapshot->last_req__ << '"';
        if (colon != begin) {
          snapshot->colon__ = colon - start;
        } else {
          snapshot->colon__ = 0;
        }
      }
      return state_;
      break;
  }
}
