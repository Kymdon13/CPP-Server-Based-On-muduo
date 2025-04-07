#include "HTTP-Response.h"

#include <string>
#include <utility>

std::string HTTPStatusToString(HTTPStatus stat) {
  switch (stat) {
    case HTTPStatus::Continue:
      return "Continue";
    case HTTPStatus::OK:
      return "OK";
    case HTTPStatus::BadRequest:
      return "Bad Request";
    case HTTPStatus::Forbidden:
      return "Forbidden";
    case HTTPStatus::NotFound:
      return "Not Found";
    case HTTPStatus::InternalServerError:
      return "Internal Server Error";
    case HTTPStatus::NotImplemented:
      return "Not Implemented";
    case HTTPStatus::HTTPVersionNotSupported:
      return "HTTP Version Not Supported";
    default:
      return "Internal Server Error";
  }
}

std::string HTTPContentTypeToString(HTTPContentType type) {
  switch (type) {
    case HTTPContentType::text_plain:
      return "text/plain";
    case HTTPContentType::text_html:
      return "text/html";
    case HTTPContentType::text_css:
      return "text/css";
    case HTTPContentType::text_javascript:
      return "text/javascript";
    case HTTPContentType::image_jpeg:
      return "image/jpeg";
    default:
      return "text/plain";
  }
}

HTTPResponse::HTTPResponse(bool close) : close_(close) {}

HTTPResponse::~HTTPResponse() {}

void HTTPResponse::SetStatus(HTTPStatus status) {
  status_ = status;
  status_string_ = std::to_string(static_cast<unsigned>(status)) + " " + HTTPStatusToString(status);
}

void HTTPResponse::SetContentType(HTTPContentType content_type) {
  AddHeader("Content-Type", HTTPContentTypeToString(content_type));
}

void HTTPResponse::AddHeader(const std::string &key, const std::string &value) { headers_[key] = value; }

void HTTPResponse::SetBody(std::string body) { body_ = std::move(body); }

bool HTTPResponse::IsClose() { return close_; }

void HTTPResponse::SetClose(bool close) { close_ = close; }

std::string HTTPResponse::GetResponse() {
  std::string msg;

  // request line
  msg += "HTTP/1.1" + status_string_ + "\r\n";

  // add connection related header
  if (close_) {
    msg += "Connection: close\r\n";
  } else {
    msg += "Connection: keep-alive\r\n";
  }

  // add other headers
  msg += "Content-Length: " + std::to_string(body_.size()) + "\r\n";
  for (const auto &header : headers_) {
    msg += header.first + ": " + header.second + "\r\n";
  }

  // add body
  msg += "\r\n";
  msg += body_;

  return msg;
}