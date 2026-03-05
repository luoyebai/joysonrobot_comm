#pragma once

// STD
#include <memory>
#include <thread>
// IDL RPC
#include "jsrcomm/idl/RpcReqMsg.hpp"
#include "jsrcomm/idl/RpcRespMsg.hpp"
// ROBOT CHANNEL
#include "jsrcomm/robot/channel/channel_publisher.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"

namespace jsr::robot::rpc {

namespace jr = jsr::robot;

class RpcServer {
   public:
    RpcServer() = default;
    ~RpcServer() = default;

    void init(const std::string& channel_name);
    void Stop();

   protected:
    virtual Response HandleRequest(Request& req) = 0;

   private:
    void DdsReqMsgHandler(const void* msg);
    int64_t SendResponse(const uint64_t uuid, const Response& resp);

    std::shared_ptr<jr::channel::ChannelPublisher<jsr::msg::RpcRespMsg>> channel_publisher_;
    std::shared_ptr<jr::channel::ChannelSubscriber<jsr::msg::RpcReqMsg>> channel_subscriber_;
};

}  // namespace jsr::robot::rpc
