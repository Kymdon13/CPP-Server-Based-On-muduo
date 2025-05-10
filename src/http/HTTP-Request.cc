#include "HTTP-Request.h"

#include <iostream>
#include <map>
#include <string>

HTTPRequest::HTTPRequest() : method_(Method::Invalid), version_(Version::Invalid), body_(""){};

HTTPRequest::~HTTPRequest(){};

void HTTPRequest::setVersion(const std::string& ver) {
  if (ver == "1.1") {
    version_ = Version::HTTP11;
  } else if (ver == "1.0") {
    version_ = Version::HTTP10;
  } else {
    version_ = Version::Invalid;
  }
}
HTTPRequest::Version HTTPRequest::version() const { return version_; }
std::string HTTPRequest::versionAsString() const {
  switch (version_) {
    case Version::HTTP11:
      return "HTTP/1.1";
      break;
    case Version::HTTP10:
      return "HTTP/1.0";
      break;
    default:
      return "Invalid";
      break;
  }
}

bool HTTPRequest::setMethod(const std::string& method) {
  if (method == "GET") {
    method_ = Method::GET;
  } else if (method == "HEAD") {
    method_ = Method::HEAD;
  } else if (method == "POST") {
    method_ = Method::POST;
  } else if (method == "OPTIONS") {
    method_ = Method::OPTIONS;
  } else {
    method_ = Method::Invalid;
  }
  return method_ != Method::Invalid;  // return false if Invalid
}
HTTPRequest::Method HTTPRequest::method() const { return method_; }
std::string HTTPRequest::methodAsString() const {
  switch (method_) {
    case Method::GET:
      return "GET";
      break;
    case Method::HEAD:
      return "HEAD";
      break;
    case Method::POST:
      return "POST";
      break;
    case Method::OPTIONS:
      return "OPTIONS";
      break;
    default:
      return "Invalid";
      break;
  }
}

void HTTPRequest::setUrl(std::string url) { url_ = std::move(url); }
const std::string& HTTPRequest::url() const { return url_; }

void HTTPRequest::addParam(const std::string& key, const std::string& value) { params_[key] = value; }
std::string HTTPRequest::getParamByKey(const std::string& key) const {
  auto it = headers_.find(key);
  // return "" if key not found
  return it == headers_.end() ? std::string() : it->second;
}
const std::unordered_map<std::string, std::string>& HTTPRequest::params() const { return params_; }

void HTTPRequest::setProtocol(std::string protocol) { protocol_ = std::move(protocol); }
const std::string& HTTPRequest::protocol() const { return protocol_; }

void HTTPRequest::addHeader(const std::string& field, const std::string& value) { headers_[field] = value; }
std::string HTTPRequest::getHeaderByKey(const std::string& field) const {
  auto it = headers_.find(field);
  // return "" if key not found
  return it == headers_.end() ? std::string() : it->second;
}
const std::unordered_map<std::string, std::string>& HTTPRequest::headers() const { return headers_; }

void HTTPRequest::setBody(const std::string& str) { body_ = str; }
void HTTPRequest::setBody(std::string&& body) { body_ = body; }
void HTTPRequest::appendBody(std::string&& body) { body_ += body; }
const std::string& HTTPRequest::body() const { return body_; }
