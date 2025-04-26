#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "HTTP-Request.h"
#include "tcp/Buffer.h"

class HTTPResponse {
 public:
  enum class Status : uint16_t {
    Continue = 100,
    OK = 200,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    InternalServerError = 500,
    NotImplemented = 501,
    HTTPVersionNotSupported = 505
  };

  enum class ContentType : uint8_t {
    text_plain,
    text_html,
    text_css,
    text_js,
    text_md,
    image_jpg,
    image_png,
    image_gif,
    image_ico,
    application_octet_stream
  };

 private:
  bool close_;
  HTTPRequest::Version version_{HTTPRequest::Version::Invalid};
  Status status_{Status::InternalServerError};
  std::unordered_map<std::string, std::string> headers_;
  // we use string to store the header
  // body_ = response body
  std::shared_ptr<Buffer> body_;
  // response_ = response line + headers + body
  std::shared_ptr<Buffer> response_;
  int big_file_fd_ = -1;

 public:
  HTTPResponse(bool close, Status status = Status::OK)
      : close_(close), status_(status), body_(nullptr), response_(nullptr) {}
  ~HTTPResponse() {}

  void setBody(std::shared_ptr<Buffer> body) { body_ = body; }
  void setStatus(Status status) { status_ = status; }
  void setContentType(ContentType content_type) { addHeader("Content-Type", contentTypeToString(content_type)); }
  void setContentType(const std::string& content_type);
  void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }
  void setBigFile(int fd) { big_file_fd_ = fd; }
  int bigFile() const { return big_file_fd_; }

  bool isClose() { return close_; }
  void setClose(bool close) { close_ = close; }

  static std::string statusToString(Status stat);
  static std::string contentTypeToString(ContentType type);

  std::shared_ptr<Buffer> getResponse();
};
