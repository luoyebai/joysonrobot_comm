#pragma once

// STD
#include <memory>
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

class RpcServer {
   public:
    using Handler = std::function<Response(const Request&)>;
    RpcServer() = default;
    virtual ~RpcServer() = default;

    void init(const std::string& channel_name);
    void stop();
    RpcServer* registerApi(int64_t api_id, Handler handler);

   protected:
    virtual Response handleRequest(Request& req) = 0;

   private:
    void ddsReqMsgHandler(const void* msg);
    int64_t sendResponse(uint64_t uuid, const Response& resp);

    std::shared_ptr<jr::channel::ChannelPublisher<jsr::msg::RpcRespMsg>> channel_publisher_;
    std::shared_ptr<jr::channel::ChannelSubscriber<jsr::msg::RpcReqMsg>> channel_subscriber_;
    std::unordered_map<int64_t, Handler> handlers_{};
};

}  // namespace jsr::robot::rpc
