#include "HTTP-Response.h"

#include <tcp/Buffer.h>

#include <memory>
#include <string>
#include <utility>

void HTTPResponse::setContentType(const std::string& content_type) {
  if (content_type == ".css") {
    setContentType(ContentType::text_css);
  } else if (content_type == ".js") {
    setContentType(ContentType::text_js);
  } else if (content_type == ".png") {
    setContentType(ContentType::image_png);
  } else if (content_type == ".jpg") {
    setContentType(ContentType::image_jpg);
  } else if (content_type == ".gif") {
    setContentType(ContentType::image_gif);
  } else if (content_type == ".ico") {
    setContentType(ContentType::image_ico);
  } else if (content_type == ".md") {
    setContentType(ContentType::text_md);
  } else {
    setContentType(ContentType::text_plain);
  }
}

std::string HTTPResponse::statusToString(Status stat) {
  switch (stat) {
    case Status::Continue:
      return "Continue";
    case Status::OK:
      return "OK";
    case Status::BadRequest:
      return "Bad Request";
    case Status::Forbidden:
      return "Forbidden";
    case Status::NotFound:
      return "Not Found";
    case Status::InternalServerError:
      return "Internal Server Error";
    case Status::NotImplemented:
      return "Not Implemented";
    case Status::HTTPVersionNotSupported:
      return "HTTP Version Not Supported";
    default:
      return "Internal Server Error";
  }
}

std::string HTTPResponse::contentTypeToString(ContentType type) {
  switch (type) {
    case ContentType::text_plain:
      return "text/plain; charset=utf-8";
    case ContentType::text_html:
      return "text/html; charset=utf-8";
    case ContentType::text_css:
      return "text/css; charset=utf-8";
    case ContentType::text_js:
      return "text/javascript; charset=utf-8";
    case ContentType::text_md:
      return "text/markdown; charset=utf-8";
    case ContentType::image_jpg:
      return "image/jpeg";
    case ContentType::image_png:
      return "image/png";
    case ContentType::image_gif:
      return "image/gif";
    case ContentType::image_ico:
      return "image/x-icon";
    case ContentType::application_json:
      return "application/json";
    case ContentType::application_octet_stream:
      return "application/octet-stream";
    default:
      return "text/plain; charset=utf-8";
  }
}

// some examples of HTTP response:
// HTTP/1.1 204 No Content
// Server: nginx
// Date: Tue, 15 Apr 2025 12:45:39 GMT
// Connection: keep-alive
// Access-Control-Allow-Credentials: true
// Access-Control-Allow-Origin: https://platform.moonshot.cn
// Vary: Origin
// Request-Context: appId=cid-v1:e97341f6-8fff-46a6-9229-fbbfe0892c78
std::shared_ptr<Buffer> HTTPResponse::getResponse() {
  std::string before_body;

  // response line
  before_body += std::string("HTTP/1.1") + ' ' + std::to_string(static_cast<unsigned>(status_)) + ' ' +
                 statusToString(status_) + "\r\n";

  // add connection related header
  if (close_) {
    before_body += "Connection: close\r\n";
  } else {
    before_body += "Connection: keep-alive\r\n";
  }

  // add other headers
  for (const auto& header : headers_) {
    before_body += header.first + ": " + header.second + "\r\n";
  }

  // if has body, add Content-Length
  if (body_) {
    before_body += "Content-Length: " + std::to_string(body_->readableBytes()) + "\r\n";
  }

  // message separator between header and body
  before_body += "\r\n";

  // construct response and add body
  size_t len;
  if (body_) {
    len = before_body.size() + body_->readableBytes() + 2;  // +2 for \r\n
  } else {
    len = before_body.size() + 2;  // +2 for \r\n
  }
  response_ = std::make_shared<Buffer>(len);
  response_->append(before_body.c_str(), before_body.size());
  if (body_) {
    response_->append(body_->peek(), body_->readableBytes());
  }
  response_->append("\r\n", 2);

  return response_;
}
