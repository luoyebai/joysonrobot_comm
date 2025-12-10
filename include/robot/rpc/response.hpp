#pragma once

#include <string>
#include <utility>
#include "robot/rpc/response_header.hpp"

namespace jsr::robot::rpc {

class Response {
   public:
    Response() = default;
    Response(const ResponseHeader& header, const std::string& body) : header_(header), body_(body) {}
    Response(ResponseHeader&& header, std::string&& body) : header_(std::move(header)), body_(std::move(body)) {}

    void SetHeader(const ResponseHeader& header) { header_ = header; }
    void SetHeader(ResponseHeader&& header) { header_ = std::move(header); }

    const ResponseHeader& GetHeader() const { return header_; }

    void SetBody(const std::string& body) { body_ = body; }
    void SetBody(std::string&& body) { body_ = std::move(body); }

    const std::string& GetBody() const { return body_; }

   private:
    ResponseHeader header_;
    std::string body_;
};
}  // namespace jsr::robot::rpc