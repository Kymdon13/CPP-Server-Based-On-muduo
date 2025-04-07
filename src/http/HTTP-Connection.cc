#include "HTTP-Connection.h"

#include <functional>
#include <utility>

#include "HTTP-Context.h"
#include "HTTP-Request.h"
#include "HTTP-Response.h"
#include "base/Exception.h"
#include "base/Utils.h"
#include "tcp/Buffer.h"
#include "tcp/TCP-Connection.h"

HTTPConnection::HTTPConnection(const std::shared_ptr<TCPConnection> &conn) {
  tcp_connection_ = conn;
  http_context_ = std::make_unique<HTTPContext>();
}

HTTPConnection::~HTTPConnection() {}

void HTTPConnection::SetResponseCallback(std::function<void(const HTTPRequest *, HTTPResponse *)> cb) {
  response_callback_ = std::move(cb);
}

void HTTPConnection::EnableHTTPConnection() {
  tcp_connection_->OnMessage([this](const std::shared_ptr<TCPConnection> &conn) {
    if (ConnectionState::Connected != conn->GetConnectionState()) {
      WarnIf(true, "TCPConnection::on_message_callback_() called while disconnected");
      return;
    }
    HTTPRequestParseState ret = http_context_->ParseRequest(conn->GetReadBuffer()->GetBuffer(),
                                                            static_cast<ssize_t>(conn->GetReadBuffer()->Size()));
    switch (ret) {
      case HTTPRequestParseState::COMPLETE: {
        HTTPRequest *req = http_context_->GetHTTPRequest();
        // check if client wish to keep the connection
        std::string conn_state = req->GetHeaderByKey("Connection");
        bool is_clnt_want_to_close = (utils::toLower(conn_state) == "close");
        // generate response
        HTTPResponse res(is_clnt_want_to_close);
        response_callback_(http_context_->GetHTTPRequest(), &res);
        // send message
        conn->Send(res.GetResponse().c_str());
        // if the client wish to close connection
        if (res.IsClose()) {
          conn->HandleClose();
        }
        // reset the state of the parser
        http_context_->ResetState();
        break;
      }
      case HTTPRequestParseState::INVALID: {
        WarnIf(true, "HTTPRequest Parsed failed");
        conn->Send(HTTPResponse(true).GetResponse());
        conn->HandleClose();
        break;
      }
      // TODO(wzy) other Invalid state to be handle
      default: {
        conn->Send(HTTPResponse(true).GetResponse());
        conn->HandleClose();
      } break;
    }
  });
}
