#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "HTTP-Request.h"
#include "tcp/Buffer.h"

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
  // we use string to store the header
  // body_ = response body
  std::shared_ptr<Buffer> body_;
  // res_ = response line + headers + body
  std::shared_ptr<Buffer> res_;

 public:
  HTTPResponse(bool close, HTTPStatus status = HTTPStatus::OK)
      : close_(close), status_(status), body_(nullptr), res_(nullptr) {}
  ~HTTPResponse() {}

  void SetStatus(HTTPStatus status) { status_ = status; }
  void SetContentType(HTTPContentType content_type) {
    AddHeader("Content-Type", HTTPContentTypeToString(content_type));
  }
  void AddHeader(const std::string &key, const std::string &value) { headers_[key] = value; }
  void SetBody(std::shared_ptr<Buffer> body) { body_ = body; }
  bool IsClose() { return close_; }
  void SetClose(bool close) { close_ = close; }

  static std::string GetStaticPath();
  static void SetStaticPath(const std::string &path);

  static std::string HTTPStatusToString(HTTPStatus stat);
  static std::string HTTPContentTypeToString(HTTPContentType type);

  std::shared_ptr<Buffer> GetResponse();
};
