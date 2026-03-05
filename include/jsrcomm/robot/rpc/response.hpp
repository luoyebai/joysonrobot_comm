#pragma once

#include <string>
#include <utility>
#include "response_header.hpp"

namespace jsr::robot::rpc {
constexpr static auto RPC_RESPONSE_CHANNEL_SUFFIX = "/response";

class Response {
   public:
    Response() = default;
    Response(const ResponseHeader& header, const std::string& body) : header_(header), body_(body) {}
    Response(ResponseHeader&& header, std::string&& body) : header_(header), body_(body) {}

    void SetHeader(const ResponseHeader& header) { header_ = header; }
    void SetHeader(ResponseHeader&& header) { header_ = header; }

    const ResponseHeader& GetHeader() const { return header_; }

    void SetBody(const std::string& body) { body_ = body; }
    void SetBody(std::string&& body) { body_ = std::move(body); }

    const std::string& GetBody() const { return body_; }

   private:
    ResponseHeader header_;
    std::string body_;
};
}  // namespace jsr::robot::rpc