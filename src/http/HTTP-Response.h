#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "HTTP-Request.h"

class HTTPResponse {
 public:
  enum class HTTPStatus : unsigned {
    Continue = 100,
    OK = 200,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    InternalServerError = 500,
    NotImplemented = 501,
    HTTPVersionNotSupported = 505
  };

  enum class HTTPContentType : uint8_t { text_plain, text_html, text_css, text_javascript, image_jpeg };

 private:
  bool close_;
  HTTPRequest::HTTPVersion version_{HTTPRequest::HTTPVersion::Invalid};
  HTTPStatus status_{HTTPStatus::InternalServerError};
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;

 public:
  HTTPResponse(bool close, HTTPStatus status = HTTPStatus::OK) : close_(close), status_(status) {}
  ~HTTPResponse() {}

  void SetStatus(HTTPStatus status);
  void SetContentType(HTTPContentType content_type);
  void AddHeader(const std::string &key, const std::string &value);
  void SetBody(std::string body);

  bool IsClose();
  void SetClose(bool close);

  std::string GetResponse();

  static std::string HTTPStatusToString(HTTPStatus stat);
  static std::string HTTPContentTypeToString(HTTPContentType type);
};