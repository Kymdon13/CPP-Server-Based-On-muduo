#pragma once

#include <string>
#include <unordered_map>

class HTTPRequest {
 public:
  enum class HTTPMethod : unsigned char { Invalid = 0, GET, HEAD, POST };
  inline std::string methodToString(HTTPMethod method) {
    switch (method) {
      case HTTPMethod::GET:
        return "GET";
      case HTTPMethod::HEAD:
        return "HEAD";
      case HTTPMethod::POST:
        return "POST";
      default:
        return "Invalid";
    }
  }

  enum class HTTPVersion : unsigned char { Invalid = 0, HTTP10, HTTP11 };
  inline std::string versionToString(HTTPVersion ver) {
    switch (ver) {
      case HTTPVersion::HTTP11:
        return "HTTP/1.1";
      case HTTPVersion::HTTP10:
        return "HTTP/1.0";
      default:
        return "Invalid";
    }
  }

 public:
  HTTPRequest();
  ~HTTPRequest();

  void SetVersion(const std::string &version);
  HTTPVersion GetVersion() const;
  std::string GetVersionString() const;

  bool SetMethod(const std::string &method);
  HTTPMethod GetMethod() const;
  std::string GetMethodString() const;

  void SetUrl(std::string url);
  const std::string &GetUrl() const;

  void SetRequestParams(const std::string &key, const std::string &value);
  std::string GetRequestValueByKey(const std::string &key) const;
  const std::unordered_map<std::string, std::string> &GetRequestParams() const;

  void SetProtocol(std::string protocol);
  const std::string &GetProtocol() const;

  void AddHeader(const std::string &field, const std::string &value);
  std::string GetHeaderByKey(const std::string &field) const;
  const std::unordered_map<std::string, std::string> &GetHeaders() const;

  void SetBody(std::string body);
  const std::string &GetBody() const;

 private:
  HTTPMethod method_{HTTPMethod::Invalid};
  std::string url_;
  HTTPVersion version_{HTTPVersion::Invalid};
  std::string protocol_;
  std::unordered_map<std::string, std::string> headers_;
  std::unordered_map<std::string, std::string> request_params_;
  std::string body_;
};
