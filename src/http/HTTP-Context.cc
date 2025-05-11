#include "HTTP-Context.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

#include "HTTP-Request.h"
#include "log/Logger.h"
#include "tcp/Buffer.h"

HTTPContext::HTTPContext()
    : contentLength_(-1),
      state_(ParseState::START),
      request_(std::make_unique<HTTPRequest>()),
      snapshot_(std::make_unique<ParsingSnapshot>()) {}

HTTPContext::~HTTPContext(){};

void HTTPContext::reset() {
  // reset the state
  contentLength_ = -1;
  state_ = ParseState::START;
  request_ = std::make_unique<HTTPRequest>();
  snapshot_->parseState__ = ParseState::START;
}

bool HTTPContext::isComplete() { return state_ == ParseState::COMPLETE; }

HTTPRequest HTTPContext::getRequest() { return *request_.get(); }

bool isInvalidURLChar(const char& c) {
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
    // case '/':
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
bool isInvalidHeaderKeyChar(const char& c) {
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

HTTPContext::ParseState HTTPContext::parseRequest(Buffer* buffer) {
  // retrieve the data from buffer
  const char* begin = buffer->peek();
  size_t size = buffer->readableBytes();
  buffer->retrieveAll();

  const char *start, *cur, *end, *colon;
  std::string combined_request;
  if (snapshot_->parseState__ > ParseState::START) {  // means not completed
    combined_request =
        snapshot_->lastRequest__ + std::string(begin, size);  // join the last request and current request
    begin = combined_request.c_str();                         // point to the left border
    start = begin;                                            // point to left border
    cur = start + snapshot_->lastRequest__.size();            // point to beginning of the next request
    end = start + combined_request.size();
    colon = start + snapshot_->colon__;  // restore the position of colon
  } else {
    start = begin;  // point to left border
    cur = start;    // point to right border
    end = start + size;
    colon = start;  // store the position of ':' if encounter 'url:params' or 'headerkey:headervalue'
  }
  while (state_ > ParseState::COMPLETE && cur < end) {
    char c = *cur;  // get current char
    switch (state_) {
      case ParseState::START: {
        if (isblank(c) || c == CR || c == LF) {  // skip with space, CR or LF
          break;
        }
        if (isupper(c)) {  // uppercase may be METHOD
          state_ = ParseState::METHOD;
          start = cur;
        } else {
          state_ = ParseState::INVALID_METHOD;  // method must be uppercase
        }
        break;
      }
      case ParseState::METHOD: {
        if (isupper(c)) {  // we are still in the METHOD
          break;
        }
        if (isblank(c)) {  // METHOD done, go to URL
          state_ = ParseState::BEFORE_URL;
          request_->setMethod(std::string(start, cur));  // string(begin, end) is [begin, end)
          start = cur + 1;
        } else {
          state_ = ParseState::INVALID_METHOD;  // method must be uppercase
        }
        break;
      }
      case ParseState::BEFORE_URL: {
        if (c == '/') {
          state_ = ParseState::URL;
        } else {
          state_ = ParseState::INVALID_URL;  // URL must start with '/'
        }
        break;
      }
      case ParseState::URL: {
        if (c == '?') {  // encounter request params
          state_ = ParseState::BEFORE_URL_PARAM_KEY;
          request_->setUrl(std::string(start, cur));  // string(begin, end) is [begin, end)
          start = cur + 1;
        } else if (isblank(c)) {  // done request params
          state_ = ParseState::BEFORE_PROTOCOL;
          request_->setUrl(std::string(start, cur));  // string(begin, end) is [begin, end)
          start = cur + 1;
        } else if (isInvalidURLChar(c)) {
          state_ = ParseState::INVALID_URL;
        }
        break;
      }
      case ParseState::BEFORE_URL_PARAM_KEY: {
        if (isInvalidURLChar(c)) {  // after '?' or '&'
          state_ = ParseState::INVALID_URL;
        } else {
          state_ = ParseState::URL_PARAM_KEY;
        }
        break;
      }
      case ParseState::URL_PARAM_KEY: {
        if (c == '=') {  // done param key, go to param value
          state_ = ParseState::BEFORE_URL_PARAM_VALUE;
          colon = cur;
        } else if (isInvalidURLChar(c)) {
          state_ = ParseState::INVALID_URL;
        }
        break;
      }
      case ParseState::BEFORE_URL_PARAM_VALUE: {
        if (isInvalidURLChar(c)) {  // after '='
          state_ = ParseState::INVALID_URL;
        } else {
          state_ = ParseState::URL_PARAM_VALUE;
        }
        break;
      }
      case ParseState::URL_PARAM_VALUE: {
        if (c == '&') {  // encounter next param
          state_ = ParseState::BEFORE_URL_PARAM_KEY;
          request_->addParam(std::string(start, colon), std::string(colon + 1, cur));
          start = cur + 1;
        } else if (isblank(c)) {  // done request params
          state_ = ParseState::BEFORE_PROTOCOL;
          request_->addParam(std::string(start, colon), std::string(colon + 1, cur));
          start = cur + 1;
        } else if (isInvalidURLChar(c)) {
          state_ = ParseState::INVALID_URL;
        }
        break;
      }
      case ParseState::BEFORE_PROTOCOL: {
        if (isupper(c)) {  // uppercase may be PROTOCOL
          state_ = ParseState::PROTOCOL;
          start = cur;
        } else {
          state_ = ParseState::INVALID_PROTOCOL;  // PROTOCOL must be uppercase
        }
        break;
      }
      case ParseState::PROTOCOL: {
        if (isupper(c)) {  // we are still in the PROTOCOL
          break;
        }
        if (c == '/') {  // done PROTOCOL, go to VERSION
          state_ = ParseState::BEFORE_VERSION;
          request_->setProtocol(std::string(start, cur));
          start = cur + 1;
        } else {
          state_ = ParseState::INVALID_PROTOCOL;  // other cases is invalid
        }
        break;
      }
      case ParseState::BEFORE_VERSION: {
        if (isdigit(c)) {
          state_ = ParseState::VERSION;
        } else {
          state_ = ParseState::INVALID_PROTOCOL;  // VERSION must has at least 1 digit
        }
        break;
      }
      case ParseState::VERSION: {
        if (c == CR) {  // done version
          state_ = ParseState::ENCOUNTER_CR;
          request_->setVersion(std::string(start, cur));
          start = cur + 1;
        } else if (isdigit(c) || c == '.') {  // only allow digit and '.' in version
          break;
        } else {
          state_ = ParseState::INVALID_PROTOCOL;
        }
        break;
      }
      case ParseState::ENCOUNTER_CR: {
        if (c == LF) {  // '\n' after '\r', means this line is over
          state_ = ParseState::CR_LF;
          start = cur + 1;
        } else {
          state_ = ParseState::INVALID_CRLF;  // CR not followed by LF
        }
        break;
      }
      case ParseState::CR_LF: {
        if (c == CR) {  // encounter blank line
          state_ = ParseState::CR_LF_CR;
        } else if (isInvalidHeaderKeyChar(c)) {  // if not CR, must be header key
          state_ = ParseState::INVALID_HEADER;
        } else {
          state_ = ParseState::HEADER_KEY;
        }
        break;
      }
      case ParseState::HEADER_KEY: {
        if (c == ':') {
          state_ = ParseState::BEFORE_HEADER_VALUE;
          colon = cur;
        } else if (isInvalidHeaderKeyChar(c)) {  // space not allowed in header key
          state_ = ParseState::INVALID_HEADER;
        }
        break;
      }
      case ParseState::BEFORE_HEADER_VALUE: {
        if (!isblank(c)) {
          state_ = ParseState::INVALID_HEADER;  // must be space between ':' and header value
        } else {
          state_ = ParseState::HEADER_VALUE;
        }
        break;
      }
      case ParseState::HEADER_VALUE: {
        if (c == CR) {
          state_ = ParseState::ENCOUNTER_CR;
          request_->addHeader(
              std::string(start, colon),
              std::string(colon + 2, cur));  // colon+2 is because there is a space between ':' and the header value
          start = cur + 1;
        }
        // we do not inspect the header value here, cuz the it varies a lot
        break;
      }
      case ParseState::CR_LF_CR: {
        if (c == LF) {
          if (request_->headers().count("Content-Length")) {  // check if there is field "Content-Length"
            if (contentLength_ = atoi(request_->getHeaderByKey("Content-Length").c_str()); contentLength_ < 0) {
              state_ = ParseState::INVALID_HEADER;
            } else if (cur - begin + 1 < static_cast<long>(size)) {  // contentLength_ >= 0 && more data
              state_ = ParseState::BODY;
            } else {                      // contentLength_ >= 0 && no more data
              if (contentLength_ == 0) {  // contentLength_ == 0 && no more data
                state_ = ParseState::COMPLETE;
              } else {  // contentLength_ > 0 && no more data, break the loop and wait for more data
                state_ = ParseState::BODY;
              }
            }
          } else {                                            // no Content-Length
            if (cur - begin + 1 < static_cast<long>(size)) {  // there is more data
              state_ = ParseState::BODY;
            } else {
              state_ = ParseState::COMPLETE;
            }
          }
          start = cur + 1;
        } else {
          state_ = ParseState::INVALID_CRLF;
        }
        break;
      }
      case ParseState::BODY: {
        size_t bodyLength;
        size_t left = size - (cur - begin);

        if (contentLength_ > left) {  // there is more body to come, break the loop
          contentLength_ -= left;
          bodyLength = left;
          cur = end - 1;                     // break the loop
        } else if (contentLength_ < left) {  // there is more body than expected, next http request arrived already,
                                             // write it back to buffer
          state_ = ParseState::COMPLETE;
          bodyLength = contentLength_;
          buffer->append(start + bodyLength, left - contentLength_);  // write back the extra data
        } else {  // contentLength_ == size - (cur - begin), complete parsing, this is also the case if Content-Length
                  // is 0
          state_ = ParseState::COMPLETE;
          bodyLength = contentLength_;
        }

        request_->appendBody(std::string(start, start + bodyLength));
        break;
      }
      default: {
        state_ = ParseState::INVALID;
        break;
      }
    }
    ++cur;
  }
  switch (state_) {
    case ParseState::COMPLETE:
      return ParseState::COMPLETE;
      break;
    case ParseState::INVALID:
      return ParseState::INVALID;
      break;
    case ParseState::INVALID_METHOD:
      return ParseState::INVALID_METHOD;
      break;
    case ParseState::INVALID_URL:
      return ParseState::INVALID_URL;
      break;
    case ParseState::INVALID_PROTOCOL:
      return ParseState::INVALID_PROTOCOL;
      break;
    case ParseState::INVALID_HEADER:
      return ParseState::INVALID_HEADER;
      break;
    case ParseState::INVALID_CRLF:
      return ParseState::INVALID_CRLF;
      break;
    default: {
      // if the state is not complete, then we need to save the state
      snapshot_->parseState__ = state_;
      if (state_ == ParseState::BODY) {
        // if body parsing not completed, no need to save the start and colon pointer,
        // this way we can speed up and save some memory
        snapshot_->lastRequest__ = "";
        snapshot_->colon__ = 0;
      } else {
        snapshot_->lastRequest__ = std::string(start, cur);
        LOG_TRACE << "HTTPContext::parseRequest: unfinished, last request: \"" << snapshot_->lastRequest__ << '"';
        if (colon != begin) {  // this means that we have at least one colon in the request
          snapshot_->colon__ = colon - start;
        } else {
          snapshot_->colon__ = 0;
        }
      }
      return state_;
      break;
    }
  }
}
