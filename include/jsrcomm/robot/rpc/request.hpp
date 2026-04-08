#pragma once

#include <string>
#include <utility>
// RPC REQ HEADER
#include "request_header.hpp"

namespace jsr::robot::rpc {
constexpr static auto RPC_REQUEST_CHANNEL_SUFFIX = "/request";

class Request {
   public:
    Request() = default;
    Request(const RequestHeader& header, const std::string& body) : header_(header), body_(body) {}
    Request(RequestHeader&& header, std::string&& body) : header_(header), body_(body) {}

    void setHeader(const RequestHeader& header) { header_ = header; }
    void setHeader(RequestHeader&& header) { header_ = header; }

    RequestHeader getHeader() const { return header_; }

    void setBody(const std::string& body) { body_ = body; }
    void setBody(std::string&& body) { body_ = std::move(body); }

    const std::string& getBody() const { return body_; }
    std::string& getBody() { return body_; }
    std::string&& moveBody() { return std::move(body_); }

   private:
    RequestHeader header_;
    std::string body_;
};

}  // namespace jsr::robot::rpc
