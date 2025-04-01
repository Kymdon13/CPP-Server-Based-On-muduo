#pragma once

#include <string>
#include <unordered_map>

enum class HTTPMethod { Invalid = 0, Get, Post, Head, Put, Delete };

enum HTTPVersion { Unknown = 0, HTTP10, HTTP11 };

class HTTPRequest {
 public:
  HTTPRequest();
  ~HTTPRequest();

  void SetVersion(const std::string &version);
  HTTPVersion GetVersion() const;
  std::string GetVersionString() const;

  bool SetMethod(const std::string &method);
  HTTPMethod GetMethod() const;
  std::string GetMethodString() const;

  void SetUrl(const std::string &url);
  const std::string &GetUrl() const;

  void SetRequestParams(const std::string &key, const std::string &value);
  std::string GetRequestValueByKey(const std::string &key) const;
  const std::unordered_map<std::string, std::string> &GetRequestParams() const;

  void SetProtocol(const std::string &protocol);
  const std::string &GetProtocol() const;

  void AddHeader(const std::string &field, const std::string &value);
  std::string GetHeaderByKey(const std::string &field) const;
  const std::unordered_map<std::string, std::string> &GetHeaders() const;

  void SetBody(const std::string &body);
  const std::string &GetBody() const;

 private:
  HTTPMethod method_{HTTPMethod::Invalid};
  std::string url_;
  HTTPVersion version_{HTTPVersion::Unknown};
  std::string protocol_;
  std::unordered_map<std::string, std::string> headers_;
  std::unordered_map<std::string, std::string> request_params_;
  std::string body_;
};
