#include "HTTP-Connection.h"

#include <functional>
#include <memory>
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

void HTTPConnection::OnResponse(std::function<void(const HTTPRequest *, HTTPResponse *)> cb) {
  on_response_callback_ = std::move(cb);
}

bool HTTPConnection::completeCallback(const std::shared_ptr<TCPConnection> &conn) {
  HTTPRequest *req = http_context_->GetHTTPRequest();
  // check if client wish to keep the connection
  std::string conn_state = req->GetHeaderByKey("Connection");
  bool is_clnt_want_to_close = (util::toLower(conn_state) == "close");
  // generate response
  HTTPResponse res(is_clnt_want_to_close);
  // set the response according to the request
  on_response_callback_(http_context_->GetHTTPRequest(), &res);
  // send message
  conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
  // reset the state of the parser and the snapshot_
  http_context_->ResetState();
  snapshot_->parsingState__ = HTTPContext::HTTPRequestParseState::INIT;
  return res.IsClose();
}

void HTTPConnection::EnableHTTPConnection() {
  tcp_connection_->OnMessage([this](const std::shared_ptr<TCPConnection> &conn) {
    if (TCPState::Connected != conn->GetConnectionState()) {
      LOG_WARN << "HTTPConnection::EnableHTTPConnection, on_message_callback_ called while disconnected";
      return;
    }
    using HTTPRequestParseState = HTTPContext::HTTPRequestParseState;
    HTTPRequestParseState return_state;
    return_state =
        http_context_->ParseRequest(conn->readBuffer()->peek(), conn->readBuffer()->readableBytes(), snapshot_.get());
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
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid HTTP METHOD";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::NotImplemented);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_URL: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid URL";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::NotFound);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_PROTOCOL: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid PROTOCOL";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::HTTPVersionNotSupported);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_HEADER: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid HEADER";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::Forbidden);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      case HTTPRequestParseState::INVALID_CRLF: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, invalid CRLF";
        HTTPResponse res(false, HTTPResponse::HTTPStatus::BadRequest);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
        break;
      }
      // this one will close the connection
      case HTTPRequestParseState::INVALID: {
        parseUnfinished_ = false;
        LOG_INFO << "HTTPConnection::EnableHTTPConnection, just invalid";
        HTTPResponse res(true, HTTPResponse::HTTPStatus::BadRequest);
        conn->Send(res.GetResponse()->peek(), res.GetResponse()->readableBytes());
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
