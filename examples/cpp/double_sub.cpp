//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// SUB
#include "jsrcomm/idl/RpcReqMsg.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"

constexpr auto LR_LOCO_TOPIC = "rt/lr/loco_ctrl";

namespace jrc = jsr::robot::channel;
using namespace jsr::msg;

void HandlerT1(const void* msg_ptr) {
    const auto* msg = static_cast<const RpcReqMsg*>(msg_ptr);
    fmt::print("[T1] Received message:header-> {}| uuid-> {}| body-> {}\n", msg->header(), msg->uuid(), msg->body());
    return;
}

void HandlerT2(const void* msg_ptr) {
    const auto* msg = static_cast<const RpcReqMsg*>(msg_ptr);
    fmt::print("[T2] Received message:header-> {}| uuid-> {}| body-> {}\n", msg->header(), msg->uuid(), msg->body());
    return;
}

int main() {
    jrc::ChannelFactory::instance()->init(0);
    jrc::ChannelSubscriber<RpcReqMsg> sub1(LR_LOCO_TOPIC, HandlerT1);
    jrc::ChannelSubscriber<RpcReqMsg> sub2(LR_LOCO_TOPIC, HandlerT2);
    sub1.initChannel();
    sub2.initChannel();

    // Wait 10 sec
    constexpr size_t SLEEP_COUNT = 10;
    constexpr size_t SLEEP_TIME = 1;
    for (size_t i = 0; i < SLEEP_COUNT; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }
    return 0;
}
