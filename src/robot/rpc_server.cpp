// STD
#include <cstdint>
// ROBOT RPC SERVER
#include "jsrcomm/robot/rpc/rpc_server.hpp"
// ROBOT CHANNEL
#include "jsrcomm/robot/channel/channel_publisher.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"
// ROBOT RPC
#include "jsrcomm/robot/rpc/error.hpp"
#include "jsrcomm/robot/rpc/request.hpp"
#include "jsrcomm/robot/rpc/response.hpp"

using namespace jsr::msg;

namespace jsr::robot::rpc {

void RpcServer::init(const std::string& channel_name) {
    channel_publisher_ =
        std::make_shared<jr::channel::ChannelPublisher<RpcRespMsg>>(channel_name + RPC_RESPONSE_CHANNEL_SUFFIX);
    channel_publisher_->initChannel();
    channel_subscriber_ =
        std::make_shared<jr::channel::ChannelSubscriber<RpcReqMsg>>(channel_name + RPC_REQUEST_CHANNEL_SUFFIX);
    channel_subscriber_->initChannel([this](const void* msg) { this->DdsReqMsgHandler(msg); });
    return;
}
void RpcServer::Stop() {
    channel_publisher_->closeChannel();
    channel_subscriber_->closeChannel();
    return;
}

void RpcServer::DdsReqMsgHandler(const void* msg) {
    const auto* req_msg = static_cast<const RpcReqMsg*>(msg);
    RequestHeader header;
    std::string body;

    try {
        nlohmann::json j = nlohmann::json::parse(req_msg->header());
        auto api_id = j.at(RPC_HEADER_JSON_APIID_KEY);
        header = RequestHeader(api_id);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Request header error: {}\n", e.what());
        header.SetApiId(jr::rpc::RPC_STATUS_CODE_INVALID);
    }

    body = req_msg->body();
    auto req = Request(header, body);
    // Use the request to call the appropriate handler
    // Like hardware, software, etc.
    auto resp = HandleRequest(req);
    SendResponse(req_msg->uuid(), resp);
    return;
}

int64_t RpcServer::SendResponse(const uint64_t uuid, const Response& resp) {
    RpcRespMsg rpc_resp_msg;
    rpc_resp_msg.uuid(uuid);
    rpc_resp_msg.header(resp.GetHeader().toJson().dump());
    rpc_resp_msg.body(resp.GetBody());
    return channel_publisher_->write(&rpc_resp_msg);
}

}  // namespace jsr::robot::rpc