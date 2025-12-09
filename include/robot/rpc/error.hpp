#pragma once

// STD
#include <cstdint>

namespace jsr::robot::rpc {

const int64_t RpcStatusCodeSuccess = 0;                  // success
const int64_t RpcStatusCodeTimeout = 100;                // request timeout
const int64_t RpcStatusCodeBadRequest = 400;             // bad request, usually when the request param is invalid
const int64_t RpcStatusCodeInternalServerError = 500;    // internal server error
const int64_t RpcStatusCodeServerRefused = 501;          // server refused the request
const int64_t RpcStatusCodeStateTransitionFailed = 502;  // robot state machine transition failed
const int64_t RpcStatusCodeInvalid = -1;  // default value, usually when the request has not been sent yet

}  // namespace jsr::robot::rpc