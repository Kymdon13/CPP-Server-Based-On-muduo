#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "HTTP-Request.h"

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

std::string HTTPStatusToString(HTTPStatus stat);

enum class HTTPContentType : unsigned short { text_plain, text_html, text_css, text_javascript, image_jpeg };

std::string HTTPContentTypeToString(HTTPContentType type);

class HTTPResponse {
 private:
  HTTPVersion version_{HTTPVersion::Invalid};
  HTTPStatus status_{HTTPStatus::InternalServerError};
  std::string status_string_;  // status_string_ = status code + status message
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
  bool close_;

 public:
  HTTPResponse(bool close);
  ~HTTPResponse();

  void SetStatus(HTTPStatus status);
  void SetContentType(HTTPContentType content_type);
  void AddHeader(const std::string &key, const std::string &value);
  void SetBody(const std::string &body);

  bool IsClose();
  void SetClose(bool close);

  std::string GetResponse();
};