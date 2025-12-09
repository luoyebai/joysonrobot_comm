#pragma once

#include <string>
#include <utility>
// RPC REQ HEADER
#include "robot/rpc/request_header.hpp"

namespace jsr::robot::rpc {

class Request {
   public:
    Request() = default;
    Request(const RequestHeader& header, const std::string& body) : header_(header), body_(body) {}
    Request(RequestHeader&& header, std::string&& body) : header_(std::move(header)), body_(std::move(body)) {}

    void SetHeader(const RequestHeader& header) { header_ = header; }
    void SetHeader(RequestHeader&& header) { header_ = std::move(header); }

    RequestHeader GetHeader() const { return header_; }

    void SetBody(const std::string& body) { body_ = body; }
    void SetBody(std::string&& body) { body_ = std::move(body); }

    const std::string& GetBody() const { return body_; }
    std::string& GetBody() { return body_; }
    std::string&& MoveBody() { return std::move(body_); }

   private:
    RequestHeader header_;
    std::string body_;
};

}  // namespace jsr::robot::rpc
