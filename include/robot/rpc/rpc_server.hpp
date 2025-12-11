#pragma once

// STD
#include <memory>
#include <thread>
// IDL RPC
#include "idl/RpcReqMsg.hpp"
#include "idl/RpcRespMsg.hpp"
// ROBOT CHANNEL
#include "robot/channel/channel_publisher.hpp"
#include "robot/channel/channel_subscriber.hpp"

namespace jsr::robot::rpc {

namespace jr = jsr::robot;

class RpcServer {
   public:
    RpcServer() = default;
    ~RpcServer() = default;

    void Init(const std::string& channel_name);
    void Stop();

   protected:
    virtual Response HandleRequest(Request& req) = 0;

   private:
    void DdsReqMsgHandler(const void* msg);
    int32_t SendResponse(const uint64_t uuid, const Response& resp);

    std::shared_ptr<jr::channel::ChannelPublisher<jsr::msg::RpcRespMsg>> channel_publisher_;
    std::shared_ptr<jr::channel::ChannelSubscriber<jsr::msg::RpcReqMsg>> channel_subscriber_;
};

}  // namespace jsr::robot::rpc
