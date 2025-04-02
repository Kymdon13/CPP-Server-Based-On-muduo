#include "HTTP-Request.h"

#include <iostream>
#include <map>
#include <string>

HTTPRequest::HTTPRequest() : method_(HTTPMethod::Invalid), version_(HTTPVersion::Invalid){};

HTTPRequest::~HTTPRequest(){};

void HTTPRequest::SetVersion(const std::string &ver) {
  if (ver == "1.1") {
    version_ = HTTPVersion::HTTP11;
  } else if (ver == "1.0") {
    version_ = HTTPVersion::HTTP10;
  } else {
    version_ = HTTPVersion::Invalid;
  }
}
HTTPVersion HTTPRequest::GetVersion() const { return version_; }
std::string HTTPRequest::GetVersionString() const {
  switch (version_) {
    case HTTPVersion::HTTP11:
      return "HTTP/1.1";
      break;
    case HTTPVersion::HTTP10:
      return "HTTP/1.0";
      break;
    default:
      return "Invalid";
      break;
  }
}

bool HTTPRequest::SetMethod(const std::string &method) {
  if (method == "GET") {
    method_ = HTTPMethod::GET;
  } else if (method == "HEAD") {
    method_ = HTTPMethod::HEAD;
  } else if (method == "POST") {
    method_ = HTTPMethod::POST;
  } else {
    method_ = HTTPMethod::Invalid;
  }
  return method_ != HTTPMethod::Invalid;  // return false if Invalid
}
HTTPMethod HTTPRequest::GetMethod() const { return method_; }
std::string HTTPRequest::GetMethodString() const {
  switch (method_) {
    case HTTPMethod::GET:
      return "GET";
      break;
    case HTTPMethod::HEAD:
      return "HEAD";
      break;
    case HTTPMethod::POST:
      return "POST";
      break;
    default:
      return "Invalid";
      break;
  }
}

void HTTPRequest::SetUrl(const std::string &url) { url_ = std::move(url); }
const std::string &HTTPRequest::GetUrl() const { return url_; }

void HTTPRequest::SetRequestParams(const std::string &key, const std::string &value) { request_params_[key] = value; }
std::string HTTPRequest::GetRequestValueByKey(const std::string &key) const {
  auto it = headers_.find(key);
  // return "" if key not found
  return it == headers_.end() ? std::string() : it->second;
}
const std::unordered_map<std::string, std::string> &HTTPRequest::GetRequestParams() const { return request_params_; }

void HTTPRequest::SetProtocol(const std::string &protocol) { protocol_ = std::move(protocol); }
const std::string &HTTPRequest::GetProtocol() const { return protocol_; }

void HTTPRequest::AddHeader(const std::string &field, const std::string &value) { headers_[field] = value; }
std::string HTTPRequest::GetHeaderByKey(const std::string &field) const {
  auto it = headers_.find(field);
  // return "" if key not found
  return it == headers_.end() ? std::string() : it->second;
}
const std::unordered_map<std::string, std::string> &HTTPRequest::GetHeaders() const { return headers_; }

void HTTPRequest::SetBody(const std::string &str) { body_ = std::move(str); }
const std::string &HTTPRequest::GetBody() const { return body_; }
