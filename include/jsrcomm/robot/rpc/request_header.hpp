#pragma once

// STD
#include <string>
// JSON
#include "serialization/json.hpp"

namespace jsr::robot::rpc {

constexpr static auto RPC_HEADER_JSON_APIID_KEY = "api_id";

class RequestHeader {
   public:
    RequestHeader() = default;
    explicit RequestHeader(int64_t api_id) : api_id_(api_id) {}

    void SetApiId(int64_t api_id) { api_id_ = api_id; }

    int64_t GetApiId() const { return api_id_; }

   public:
    void fromJson(nlohmann::json& json) { api_id_ = json[RPC_HEADER_JSON_APIID_KEY]; }

    nlohmann::json toJson() const {
        auto json = nlohmann::json();
        json[RPC_HEADER_JSON_APIID_KEY] = api_id_;
        return json;
    }

   private:
    int64_t api_id_ = 0;
};

}  // namespace jsr::robot::rpc
