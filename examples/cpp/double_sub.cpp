//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// SUB
#include "idl/RpcReqMsg.hpp"
#include "robot/channel/channel_subscriber.hpp"

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
    jrc::ChannelFactory::Instance()->Init(0);
    jrc::ChannelSubscriber<RpcReqMsg> sub1(LR_LOCO_TOPIC, HandlerT1);
    jrc::ChannelSubscriber<RpcReqMsg> sub2(LR_LOCO_TOPIC, HandlerT2);
    sub1.InitChannel();
    sub2.InitChannel();

    // Wait 10 sec
    for (size_t i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
