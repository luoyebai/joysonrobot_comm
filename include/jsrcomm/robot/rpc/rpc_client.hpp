#pragma once
// STD
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
// IDL RPC
#include "jsrcomm/idl/RpcReqMsg.hpp"
#include "jsrcomm/idl/RpcRespMsg.hpp"
// ROBOT CHANNEL
#include "jsrcomm/robot/channel/channel_publisher.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"
// ROBOT RPC
#include "request.hpp"
#include "response.hpp"

namespace jsr::robot::rpc {

namespace jr = jsr::robot;

constexpr bool JSR_DDS_RPC_PRINT_EXCEPTION = false;

class RpcClient {
   public:
    RpcClient() = default;
    ~RpcClient() = default;

    void init(const std::string& channel_name);
    Response sendApiRequest(const Request& req, int64_t timeout_ms = 1000);
    void sendApiRequestAsync(const Request& req, std::function<void(Response)> cb);

    void stop();

    uint64_t genUuid();

   private:
    void ddsSubMsgHandler(const void* msg);

    struct SyncEntry {
        std::promise<Response> prom;
    };

    std::mutex async_mutex_;
    std::unordered_map<uint64_t, std::function<void(Response)>> async_cb_map_;
    std::mutex mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<SyncEntry>> resp_map_;

    std::shared_ptr<jr::channel::ChannelPublisher<jsr::msg::RpcReqMsg>> channel_publisher_;
    std::shared_ptr<jr::channel::ChannelSubscriber<jsr::msg::RpcRespMsg>> channel_subscriber_;
};

}  // namespace jsr::robot::rpc