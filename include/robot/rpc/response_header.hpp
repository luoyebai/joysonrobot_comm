#pragma once

// STD
#include <string>
// JSON
#include "serialization/json.hpp"

namespace jsr::robot::rpc {

class ResponseHeader {
   public:
    ResponseHeader() = default;
    explicit ResponseHeader(const int32_t status) : status_(status) {}

    void SetStatus(const int32_t status) { status_ = status; }

    int32_t GetStatus() const { return status_; }

   public:
    void FromJson(nlohmann::json& json) { status_ = json["status"]; }

    nlohmann::json ToJson() const {
        auto json = nlohmann::json();
        json["status"] = status_;
        return json;
    }

   private:
    int32_t status_ = -1;
};

}  // namespace jsr::robot::rpc
