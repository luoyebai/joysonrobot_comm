// STD
#include <cstdint>
// ROBOT RPC SERVER
#include "robot/rpc/rpc_server.hpp"
// ROBOT CHANNEL
#include "robot/channel/channel_publisher.hpp"
#include "robot/channel/channel_subscriber.hpp"
// ROBOT RPC
#include "robot/rpc/error.hpp"
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"

using namespace jsr::msg;

namespace jsr::robot::rpc {
void RpcServer::Init(const std::string& channel_name) {
    channel_publisher_ = std::make_shared<jr::channel::ChannelPublisher<RpcRespMsg>>(channel_name + "/response");
    channel_publisher_->InitChannel();
    channel_subscriber_ = std::make_shared<jr::channel::ChannelSubscriber<RpcReqMsg>>(channel_name + "/request");
    channel_subscriber_->InitChannel([this](const void* msg) { this->DdsReqMsgHandler(msg); });
    return;
}
void RpcServer::Stop() {
    channel_publisher_->CloseChannel();
    channel_subscriber_->CloseChannel();
    return;
}

void RpcServer::DdsReqMsgHandler(const void* msg) {
    const auto* req_msg = static_cast<const RpcReqMsg*>(msg);
    RequestHeader header;
    std::string body;

    try {
        nlohmann::json j = nlohmann::json::parse(req_msg->header());
        auto api_id = j.at("api_id");
        header = RequestHeader(api_id);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Request header error: {}\n", e.what());
        header.SetApiId(jr::rpc::RpcStatusCodeInvalid);
    }

    body = req_msg->body();
    auto req = Request(header, body);
    // Use the request to call the appropriate handler
    // Like hardware, software, etc.
    auto resp = HandleRequest(req);
    SendResponse(req_msg->uuid(), resp);
    return;
}

int32_t RpcServer::SendResponse(const std::string& uuid, const Response& resp) {
    RpcRespMsg rpc_resp_msg;
    rpc_resp_msg.uuid(uuid);
    rpc_resp_msg.header(resp.GetHeader().ToJson().dump());
    rpc_resp_msg.body(resp.GetBody());
    return channel_publisher_->Write(&rpc_resp_msg);
}

}  // namespace jsr::robot::rpc