#include "HTTP-Request.h"

#include <iostream>
#include <map>
#include <string>

HTTPRequest::HTTPRequest() : method_(HTTPMethod::Invalid), version_(HTTPVersion::Unknown){};

HTTPRequest::~HTTPRequest(){};

void HTTPRequest::SetVersion(const std::string &ver) {
  if (ver == "1.1") {
    version_ = HTTPVersion::HTTP11;
  } else if (ver == "1.0") {
    version_ = HTTPVersion::HTTP10;
  } else {
    version_ = HTTPVersion::Unknown;
  }
}
HTTPVersion HTTPRequest::GetVersion() const { return version_; }
std::string HTTPRequest::GetVersionString() const {
  std::string version;
  if (version_ == HTTPVersion::HTTP11) {
    version = "http1.1";
  } else if (version_ == HTTPVersion::HTTP10) {
    version = "http1.0";
  } else {
    version = "unknown";
  }
  return version;
}

bool HTTPRequest::SetMethod(const std::string &method) {
  if (method == "GET") {
    method_ = HTTPMethod::Get;
  } else if (method == "POST") {
    method_ = HTTPMethod::Post;
  } else if (method == "HEAD") {
    method_ = HTTPMethod::Head;
  } else if (method == "PUT") {
    method_ = HTTPMethod::Put;
  } else if (method == "Delete") {
    method_ = HTTPMethod::Delete;
  } else {
    method_ = HTTPMethod::Invalid;
  }
  // return false if Invalid
  return method_ != HTTPMethod::Invalid;
}
HTTPMethod HTTPRequest::GetMethod() const { return method_; }
std::string HTTPRequest::GetMethodString() const {
  std::string method;
  if (method_ == HTTPMethod::Get) {
    method = "GET";
  } else if (method_ == HTTPMethod::Post) {
    method = "POST";
  } else if (method_ == HTTPMethod::Head) {
    method = "HEAD";
  } else if (method_ == HTTPMethod::Put) {
    method = "PUT";
  } else if (method_ == HTTPMethod::Delete) {
    method = "DELETE";
  } else {
    method = "INVALID";
  }
  return method;
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
