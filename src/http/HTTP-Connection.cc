#include "HTTP-Connection.h"

#include <functional>
#include <utility>

#include "HTTP-Context.h"
#include "HTTP-Request.h"
#include "HTTP-Response.h"
#include "base/Exception.h"
#include "base/Util.h"
#include "log/Logger.h"
#include "tcp/Buffer.h"
#include "tcp/TCP-Connection.h"

HTTPConnection::HTTPConnection(const std::shared_ptr<TCPConnection> &conn) {
  tcp_connection_ = conn;
  snapshot_ = std::make_unique<HTTPContext::parsingSnapshot>();
  http_context_ = std::make_unique<HTTPContext>();
}

HTTPConnection::~HTTPConnection() {}

void HTTPConnection::OnMessage(std::function<void(const HTTPRequest *, HTTPResponse *)> cb) {
  on_message_callback_ = std::move(cb);
}

bool HTTPConnection::completeCallback(const std::shared_ptr<TCPConnection> &conn) {
  HTTPRequest *req = http_context_->GetHTTPRequest();
  // check if client wish to keep the connection
  std::string conn_state = req->GetHeaderByKey("Connection");
  bool is_clnt_want_to_close = (util::toLower(conn_state) == "close");
  // generate response
  HTTPResponse res(is_clnt_want_to_close);
  on_message_callback_(http_context_->GetHTTPRequest(), &res);
  // send message
  conn->Send(res.GetResponse().c_str());
  // reset the state of the parser
  http_context_->ResetState();
  return res.IsClose();
}

void HTTPConnection::EnableHTTPConnection() {
  tcp_connection_->OnMessage([this](const std::shared_ptr<TCPConnection> &conn) {
    if (TCPState::Connected != conn->GetConnectionState()) {
      WarnIf(true, "TCPConnection::on_message_callback_() called while disconnected");
      return;
    }
    using HTTPRequestParseState = HTTPContext::HTTPRequestParseState;
    HTTPRequestParseState return_state;
    return_state = http_context_->ParseRequest(conn->readBuffer()->peek(), conn->readBuffer()->readableBytes(), snapshot_.get());
    conn->readBuffer()->retrieveAll();
    switch (return_state) {
      case HTTPRequestParseState::COMPLETE: {
        parseUnfinished_ = false;
        bool is_close = completeCallback(conn);
        // if the client wish to close connection
        if (is_close) {
          if (conn->GetConnectionState() == TCPState::Disconnected) {
            break;
          }
          conn->HandleClose();
        }
        break;
      }
      // handle invalid cases
      case HTTPRequestParseState::INVALID_METHOD: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPContext::ParseRequest: invalid HTTP METHOD";
        conn->Send(HTTPResponse(false, HTTPResponse::HTTPStatus::NotImplemented).GetResponse());
        break;
      }
      case HTTPRequestParseState::INVALID_URL: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPContext::ParseRequest: invalid URL";
        conn->Send(HTTPResponse(false, HTTPResponse::HTTPStatus::NotFound).GetResponse());
        break;
      }
      case HTTPRequestParseState::INVALID_PROTOCOL: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPContext::ParseRequest: invalid PROTOCOL";
        conn->Send(HTTPResponse(false, HTTPResponse::HTTPStatus::HTTPVersionNotSupported).GetResponse());
        break;
      }
      case HTTPRequestParseState::INVALID_HEADER: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPContext::ParseRequest: invalid HEADER";
        conn->Send(HTTPResponse(false, HTTPResponse::HTTPStatus::Forbidden).GetResponse());
        break;
      }
      case HTTPRequestParseState::INVALID_CRLF: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPContext::ParseRequest: invalid CRLF";
        conn->Send(HTTPResponse(false, HTTPResponse::HTTPStatus::BadRequest).GetResponse());
        break;
      }
      // this one will close the connection
      case HTTPRequestParseState::INVALID: {
        parseUnfinished_ = false;
        WarnIf(true, "HTTPRequest Parsed failed");
        conn->Send(HTTPResponse(true).GetResponse());
        if (conn->GetConnectionState() == TCPState::Disconnected) {
          break;
        }
        conn->HandleClose();
        break;
      }
      // UNFINISHED, waiting for more data
      default: {
        parseUnfinished_ = true;
        break;
      }
    }
  });
}
