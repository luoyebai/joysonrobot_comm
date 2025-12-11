// STD
#include <random>
// ROBOT RPC CLIENT
#include "robot/rpc/rpc_client.hpp"
// ROBOT CHANNEL
#include "robot/channel/channel_publisher.hpp"
#include "robot/channel/channel_subscriber.hpp"
// ROBOT RPC
#include "robot/rpc/error.hpp"
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"

using namespace jsr::msg;

namespace jsr::robot::rpc {

constexpr static size_t MAP_RESERVE_INIT = 1024;
constexpr static auto RPC_REQUEST_CHANNEL_SUFFIX = "/request";
constexpr static auto RPC_RESPONSE_CHANNEL_SUFFIX = "/response";

void RpcClient::Init(const std::string& channel_name) {
    channel_publisher_ =
        std::make_shared<jr::channel::ChannelPublisher<RpcReqMsg>>(channel_name + RPC_REQUEST_CHANNEL_SUFFIX);
    channel_subscriber_ =
        std::make_shared<jr::channel::ChannelSubscriber<RpcRespMsg>>(channel_name + RPC_RESPONSE_CHANNEL_SUFFIX);
    channel_publisher_->InitChannel();
    channel_subscriber_->InitChannel([this](const void* msg) { this->DdsSubMsgHandler(msg); });

    resp_map_.reserve(MAP_RESERVE_INIT);
    async_cb_map_.reserve(MAP_RESERVE_INIT);

    return;
}

Response RpcClient::SendApiRequest(const Request& req, int64_t timeout_ms) {
    const auto uuid = this->GenUuid();
    auto entry = std::make_shared<SyncEntry>();
    // lock map
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resp_map_[uuid] = entry;
    }
    // send
    Response resp;
    RpcReqMsg req_msg;
    req_msg.header(req.GetHeader().ToJson().dump());
    req_msg.body(req.GetBody());
    req_msg.uuid(uuid);
    channel_publisher_->Write(&req_msg);

    // get response
    auto future = entry->prom.get_future();
    // wait for response
    if (future.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::timeout) {
        // timeout
        resp.SetHeader(ResponseHeader(RPC_STATUS_CODE_TIMEOUT));
    } else {
        resp = future.get();
    }

    switch (resp.GetHeader().GetStatus()) {
        case jr::rpc::RPC_STATUS_CODE_SUCCESS:
            break;
        case jr::rpc::RPC_STATUS_CODE_TIMEOUT:
            fmt::print(stderr, "Rpc client response timeout\n");
            break;
        case jr::rpc::RPC_STATUS_CODE_BAD_REQUEST:
            fmt::print(stderr, "Rpc client response code bad\n");
            break;
        case jr::rpc::RPC_STATUS_CODE_INTERNAL_SERVER_ERROR:
            fmt::print(stderr, "Rpc client response internal server error\n");
            break;
        case jr::rpc::RPC_STATUS_CODE_SERVER_REFUSED:
            fmt::print(stderr, "Rpc client response server refused\n");
            break;
        case jr::rpc::RPC_STATUS_CODE_STATE_TRANSITION_FAILED:
            fmt::print(stderr, "Rpc client response transition failed\n");
            break;
        case jr::rpc::RPC_STATUS_CODE_INVALID:
            fmt::print(stderr, "Rpc client response code invalid\n");
            break;
        default:
            fmt::print(stderr, "Rpc client response code unknown\n");
            break;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        resp_map_.erase(uuid);
    }

    return resp;
}

void RpcClient::SendApiRequestAsync(const Request& req, std::function<void(Response)> cb) {
    const auto uuid = GenUuid();
    // lock map
    {
        std::lock_guard<std::mutex> lock(mutex_);
        async_cb_map_[uuid] = std::move(cb);
    }
    // send
    RpcReqMsg msg;
    msg.uuid(uuid);
    msg.header(req.GetHeader().ToJson().dump());
    msg.body(req.GetBody());
    channel_publisher_->Write(&msg);
}

void RpcClient::Stop() {
    channel_publisher_->CloseChannel();
    channel_subscriber_->CloseChannel();
    return;
}

uint64_t RpcClient::GenUuid() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    return dist(rng);
}

void RpcClient::DdsSubMsgHandler(const void* msg) {
    const auto* resp_msg = static_cast<const RpcRespMsg*>(msg);
    // const std::string& uuid = resp_msg->uuid();
    const auto uuid = resp_msg->uuid();
    ResponseHeader header;
    std::string body;

    try {
        const auto& j = nlohmann::json::parse(resp_msg->header());
        const auto status = j.at(RPC_HEADER_JSON_STATUS_KEY);
        header = ResponseHeader(status);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Response header error:{}\n", e.what());
        header.SetStatus(jr::rpc::RPC_STATUS_CODE_INVALID);
    }
    body = resp_msg->body();

    // sync lock and set
    std::shared_ptr<SyncEntry> entry;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = resp_map_.find(uuid);
        if (it != resp_map_.end()) {
            entry = it->second;
            resp_map_.erase(it);
        }
    }
    if (entry) {
        entry->prom.set_value(Response(header, body));
        return;
    }

    // async callback
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = async_cb_map_.find(uuid);
        if (it != async_cb_map_.end()) {
            auto cb = it->second;
            async_cb_map_.erase(it);
            cb(Response(header, body));
            return;
        }
    }

    return;
}

}  // namespace jsr::robot::rpc