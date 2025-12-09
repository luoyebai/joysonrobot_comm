// STD
#include <condition_variable>
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

void RpcClient::Init(const std::string& channel_name) {
    channel_publisher_ = std::make_shared<jr::channel::ChannelPublisher<RpcReqMsg>>(channel_name + "/request");
    channel_publisher_->InitChannel();
    channel_subscriber_ = std::make_shared<jr::channel::ChannelSubscriber<RpcRespMsg>>(channel_name + "/response");
    channel_subscriber_->InitChannel([this](const void* msg) { this->DdsSubMsgHandler(msg); });

    resp_map_.reserve(1024);
    async_map_.reserve(1024);

    return;
}

Response RpcClient::SendApiRequest(const Request& req, int64_t timeout_ms) {
    Response resp;
    RpcReqMsg req_msg;
    const auto uuid = this->GenUuid();
    req_msg.header(req.GetHeader().ToJson().dump());
    req_msg.body(req.GetBody());
    req_msg.uuid(uuid);

    // send request
    channel_publisher_->Write(&req_msg);

    std::unique_lock<std::mutex> lock(mutex_);

    // wait for response
    if (auto it = resp_map_.find(uuid); it != resp_map_.end()) {
        // Response already exists
        resp = it->second.first;
    } else {
        resp_map_[uuid] = std::make_pair(Response(), std::make_unique<std::condition_variable>());
        if (resp_map_[uuid].second->wait_for(lock, std::chrono::milliseconds(timeout_ms)) == std::cv_status::timeout) {
            resp_map_[uuid].first.SetHeader(ResponseHeader(RpcStatusCodeTimeout));
        }
        resp = resp_map_[uuid].first;
    }

    switch (resp.GetHeader().GetStatus()) {
        case jr::rpc::RpcStatusCodeSuccess:
            break;
        case jr::rpc::RpcStatusCodeTimeout:
            fmt::print(stderr, "Rpc client response timeout\n");
            break;
        case jr::rpc::RpcStatusCodeBadRequest:
            fmt::print(stderr, "Rpc client response code bad\n");
            break;
        case jr::rpc::RpcStatusCodeInternalServerError:
            fmt::print(stderr, "Rpc client response internal server error\n");
            break;
        case jr::rpc::RpcStatusCodeServerRefused:
            fmt::print(stderr, "Rpc client response server refused\n");
            break;
        case jr::rpc::RpcStatusCodeStateTransitionFailed:
            fmt::print(stderr, "Rpc client response transition failed\n");
            break;
        case jr::rpc::RpcStatusCodeInvalid:
            fmt::print(stderr, "Rpc client response code invalid\n");
            break;
        default:
            fmt::print(stderr, "Rpc client response code unknown\n");
            break;
    }

    resp_map_.erase(uuid);
    return resp;
}

void RpcClient::SendApiRequestAsync(const Request& req, std::function<void(Response)> cb) {
    const auto uuid = GenUuid();
    RpcReqMsg msg;
    msg.uuid(uuid);
    msg.header(req.GetHeader().ToJson().dump());
    msg.body(req.GetBody());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        async_map_[uuid] = cb;
    }
    channel_publisher_->Write(&msg);
}

void RpcClient::Stop() {
    channel_publisher_->CloseChannel();
    channel_subscriber_->CloseChannel();
    return;
}

std::string RpcClient::GenUuid() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    std::stringstream ss;
    ss << std::hex << dist(rng);
    return ss.str();
}

void RpcClient::DdsSubMsgHandler(const void* msg) {
    const auto* resp_msg = static_cast<const RpcRespMsg*>(msg);
    const std::string& uuid = resp_msg->uuid();
    ResponseHeader header;
    std::string body;

    try {
        nlohmann::json j = nlohmann::json::parse(resp_msg->header());
        auto status = j.at("status");
        header = ResponseHeader(status);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Response header error:{}\n", e.what());
        header.SetStatus(jr::rpc::RpcStatusCodeInvalid);
    }

    body = resp_msg->body();

    std::unique_lock<std::mutex> lock(mutex_);
    if (auto it = async_map_.find(uuid); it != async_map_.end()) {
        auto cb = it->second;
        cb(Response(header, body));
        async_map_.erase(it);
        lock.unlock();
        return;
    }

    if (auto it = resp_map_.find(uuid); it != resp_map_.end()) {
        it->second.first.SetHeader(header);
        it->second.first.SetBody(body);
        it->second.second->notify_one();
    } else {
        // Response before request locked
        resp_map_[uuid] = std::make_pair(Response(header, body), std::make_unique<std::condition_variable>());
    }
}

}  // namespace jsr::robot::rpc