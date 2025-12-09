#pragma once

// STD
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
// IDL RPC
#include "idl/RpcReqMsg.hpp"
#include "idl/RpcRespMsg.hpp"
// ROBOT CHANNEL
#include "robot/channel/channel_publisher.hpp"
#include "robot/channel/channel_subscriber.hpp"
// ROBOT RPC
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"

namespace jsr::robot::rpc {

namespace jr = jsr::robot;

class RpcClient {
   public:
    RpcClient() = default;
    ~RpcClient() = default;

    void Init(const std::string& channel_name);
    Response SendApiRequest(const Request& req, int64_t timeout_ms = 1000);

    void Stop();

    std::string GenUuid();

   private:
    void DdsSubMsgHandler(const void* msg);

    std::mutex mutex_;
    std::unordered_map<std::string, std::pair<Response, std::unique_ptr<std::condition_variable>>> resp_map_;

    std::shared_ptr<jr::channel::ChannelPublisher<jsr::msg::RpcReqMsg>> channel_publisher_;
    std::shared_ptr<jr::channel::ChannelSubscriber<jsr::msg::RpcRespMsg>> channel_subscriber_;
};

}  // namespace jsr::robot::rpc