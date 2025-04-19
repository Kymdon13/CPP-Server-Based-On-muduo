#pragma once

#include <string>
#include <unordered_map>

class HTTPRequest {
 public:
  enum class Method : uint8_t { Invalid = 0, GET, HEAD, POST };
  inline std::string methodToString(Method method) {
    switch (method) {
      case Method::GET:
        return "GET";
      case Method::HEAD:
        return "HEAD";
      case Method::POST:
        return "POST";
      default:
        return "Invalid";
    }
  }

  enum class Version : uint8_t { Invalid = 0, HTTP10, HTTP11 };
  inline std::string versionToString(Version ver) {
    switch (ver) {
      case Version::HTTP11:
        return "HTTP/1.1";
      case Version::HTTP10:
        return "HTTP/1.0";
      default:
        return "Invalid";
    }
  }

 public:
  HTTPRequest();
  ~HTTPRequest();

  void setVersion(const std::string& version);
  Version version() const;
  std::string versionAsString() const;

  bool setMethod(const std::string& method);
  Method method() const;
  std::string methodAsString() const;

  void setUrl(std::string url);
  const std::string& url() const;

  void addParam(const std::string& key, const std::string& value);
  std::string getParamByKey(const std::string& key) const;
  // return the raw map of <key, value> of request params
  const std::unordered_map<std::string, std::string>& params() const;

  void setProtocol(std::string protocol);
  const std::string& protocol() const;

  void addHeader(const std::string& field, const std::string& value);
  std::string getHeaderByKey(const std::string& field) const;
  const std::unordered_map<std::string, std::string>& headers() const;

  void setBody(const std::string& body);
  void setBody(std::string&& body);
  void appendBody(std::string&& body);
  const std::string& body() const;

 private:
  Method method_{Method::Invalid};
  std::string url_;
  Version version_{Version::Invalid};
  std::string protocol_;
  std::unordered_map<std::string, std::string> params_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};
