#pragma once

// STD
#include <cstdint>

namespace jsr::robot::rpc {

constexpr int64_t RPC_STATUS_CODE_SUCCESS = 0;        // success
constexpr int64_t RPC_STATUS_CODE_TIMEOUT = 100;      // request timeout
constexpr int64_t RPC_STATUS_CODE_BAD_REQUEST = 400;  // bad request, usually when the request param is invalid
constexpr int64_t RPC_STATUS_CODE_INTERNAL_SERVER_ERROR = 500;    // internal server error
constexpr int64_t RPC_STATUS_CODE_SERVER_REFUSED = 501;           // server refused the request
constexpr int64_t RPC_STATUS_CODE_STATE_TRANSITION_FAILED = 502;  // robot state machine transition failed
constexpr int64_t RPC_STATUS_CODE_INVALID = -1;  // default value, usually when the request has not been sent yet

}  // namespace jsr::robot::rpc