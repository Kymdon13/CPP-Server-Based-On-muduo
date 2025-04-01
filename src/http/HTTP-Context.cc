#include "HTTP-Context.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

#include "HTTP-Request.h"

HTTPContext::HTTPContext() : state_(HTTPRequestParseState::START) { http_request_ = std::make_unique<HTTPRequest>(); }

HTTPContext::~HTTPContext(){};

void HTTPContext::ResetState() { state_ = HTTPRequestParseState::START; }

bool HTTPContext::IsComplete() { return state_ == HTTPRequestParseState::COMPLETE; }

HTTPRequest *HTTPContext::GetHTTPRequest() { return http_request_.get(); }

bool HTTPContext::ParseRequest(const char *begin, ssize_t size) {
  char *start = const_cast<char *>(begin);  // point to left border
  char *cur = start;  // point to right border
  char *colon = cur;  // store the position of ':' if encounter 'url:params' or 'headerkey:headervalue'
  while (state_ != HTTPRequestParseState::INVALID && state_ != HTTPRequestParseState::COMPLETE && cur - begin < size) {
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
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::METHOD: {
        if (isupper(c)) {  // we are still in the METHOD
          break;
        }
        if (isblank(c)) {  // METHOD done, go to URL
          state_ = HTTPRequestParseState::BEFORE_URL;
          http_request_->SetMethod(std::string(start, cur));  // string(begin, end) is left-closed, right-open
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_URL: {
        if (isblank(c)) {
          break;
        }
        if (c == '/') {  // go to URL
          state_ = HTTPRequestParseState::URL;
        } else {
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::URL: {
        if (c == '?') { // encounter request params
          state_ = HTTPRequestParseState::BEFORE_URL_PARAM_KEY;
          http_request_->SetUrl(std::string(start, cur));
          start = cur + 1;
        } else if (isblank(c)) {  // done request params
          state_ = HTTPRequestParseState::BEFORE_PROTOCOL;
          http_request_->SetUrl(std::string(start, cur));
          start = cur + 1;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_URL_PARAM_KEY: {
        if (isblank(c) || c == CR || c == LF) {  // space, CR and LF invalid when in request params
          state_ = HTTPRequestParseState::INVALID;
        } else {
          state_ = HTTPRequestParseState::URL_PARAM_KEY;
        }
        break;
      }
      case HTTPRequestParseState::URL_PARAM_KEY: {
        if (c == '=') {  // done param key, go to param value
          state_ = HTTPRequestParseState::BEFORE_URL_PARAM_VALUE;
          colon = cur;
        } else if (isblank(c)) {
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_URL_PARAM_VALUE: {
        if (isblank(c) || c == CR || c == LF) {  // space, CR and LF invalid when in request params
          state_ = HTTPRequestParseState::INVALID;
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
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_PROTOCOL: {
        if (isblank(c)) {  // skip space before the protocol
          break;
        }
        state_ = HTTPRequestParseState::PROTOCOL;
        start = cur;
        break;
      }
      case HTTPRequestParseState::PROTOCOL: {
        if (c == '/') {
          state_ = HTTPRequestParseState::BEFORE_VERSION;
          http_request_->SetProtocol(std::string(start, cur));
          start = cur + 1;
        }
        break;
      }
      case HTTPRequestParseState::BEFORE_VERSION: {
        if (isdigit(c)) {  // done protocol, go to version
          state_ = HTTPRequestParseState::VERSION;
        } else {
          state_ = HTTPRequestParseState::INVALID;
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
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::ENCOUNTER_CR: {
        if (c == LF) {  // '\n' after '\r', means this line is over
          state_ = HTTPRequestParseState::CR_LF;
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::CR_LF: {
        if (c == CR) {  // encounter blank line
          state_ = HTTPRequestParseState::CR_LF_CR;
        } else if (isblank(c)) {
          state_ = HTTPRequestParseState::INVALID;
        } else {
          state_ = HTTPRequestParseState::HEADER_KEY;
        }
        break;
      }
      case HTTPRequestParseState::HEADER_KEY: {
        if (c == ':') {
          state_ = HTTPRequestParseState::HEADER_VALUE;
          colon = cur;
        }
        break;
      }
      case HTTPRequestParseState::HEADER_VALUE: {
        if (isblank(c)) {
          break;
        } else if (c == CR) {
          state_ = HTTPRequestParseState::ENCOUNTER_CR;
          http_request_->AddHeader(std::string(start, colon), std::string(colon + 2, cur));  // colon+2 is because there is a space between ':' and the header value
          start = cur + 1;
        }
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
            if (cur - begin + 1 < size) {  // if LF is the last char, then cur-begin = size-1
              state_ = HTTPRequestParseState::BODY;
            } else {
              state_ = HTTPRequestParseState::COMPLETE;
            }
          }
          start = cur + 1;
        } else {
          state_ = HTTPRequestParseState::INVALID;
        }
        break;
      }
      case HTTPRequestParseState::BODY: {
        state_ = HTTPRequestParseState::COMPLETE;
        // if "Content-Length" is presented, use it as body length
        size_t bodylength = std::min(static_cast<long>(content_length_), size - (cur - begin));
        http_request_->SetBody(std::string(start, start + bodylength));
        break;
      }
      default:
        state_ = HTTPRequestParseState::INVALID;
        break;
    }
    ++cur;
  }
  return state_ == HTTPRequestParseState::COMPLETE;
}
