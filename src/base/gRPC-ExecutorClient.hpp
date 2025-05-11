#pragma once
#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>

#include "../../build/executor.grpc.pb.h"
#include "base/json.hpp"
#include "http/HTTP-Response.h"
#include "log/Logger.h"
#include "tcp/TCP-Connection.h"

namespace grpc {

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using executor::Executor;
using executor::Req;
using executor::Res;

Req MakeReq(const std::string& code, int edition) {
  Req req;
  req.set_edition(edition);
  req.set_code(code);
  return req;
}

/* Async version */
class ExecutorClient {
 public:
  ExecutorClient(std::shared_ptr<Channel> channel) : stub_(Executor::NewStub(channel)) {}

  /* note that the method will return immediately */
  void GetResult(const Req& exec_req, std::shared_ptr<TCPConnection> conn, HTTPResponse* res) {
    Res* exec_res = new Res();
    ClientContext* context = new ClientContext();

    stub_->async()->GetResult(context, &exec_req, exec_res, [exec_res, context, conn, res](Status status) {
      /* deal with the exec_res */
      if (!status.ok()) {
        LOG_ERROR << "gRPC call failed: " << status.error_message();
        res->setClose(true);
        res->setStatus(HTTPResponse::Status::BadRequest);
        res->setBody(std::make_shared<Buffer>("Invalid JSON"));
      } else {
        res->setStatus(HTTPResponse::Status::OK);
        res->setContentType(HTTPResponse::ContentType::application_json);
        res->addHeader("access-control-allow-origin", "*");
        // serialize json as response
        nlohmann::ordered_json json_res;

        // std::cout << "=======================================================" << std::endl;
        // std::cout << "compile_result: " << exec_res->c_stdout_str() << std::endl;
        // std::cout << "compile_error: " << exec_res->c_stderr_str() << std::endl;
        // std::cout << "result: " << exec_res->stdout_str() << std::endl;
        // std::cout << "error: " << exec_res->stderr_str() << std::endl;

        json_res["compile_result"] = exec_res->c_stdout_str();
        json_res["compile_error"] = exec_res->c_stderr_str();
        json_res["result"] = exec_res->stdout_str();
        json_res["error"] = exec_res->stderr_str();
        res->setBody(std::make_shared<Buffer>(json_res.dump()));
      }

      /* we must call conn->send by hand,
      cuz it was handled by TCPServer::on_message_callback_ in the none callback situation */
      conn->send(res->getResponse()->peek(), res->getResponse()->readableBytes());

      // close the connection if needed
      if (res->isClose() && conn->connectionState() != TCPState::Disconnected) {
        conn->handleClose();
      }

      /* we have to do the work left in TCPConnection::on_message_callback_ */
      delete context;
      delete exec_res;
      delete res;
    });
  }

 private:
  std::unique_ptr<Executor::Stub> stub_;
};

}  // namespace grpc