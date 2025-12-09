//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// IDL
#include "idl/LowState.hpp"
// SUB
#include "robot/channel/channel_subscriber.hpp"

constexpr auto TOPIC = "rt/low_state";

namespace jrc = jsr::robot::channel;
using namespace jsr::msg;

void Handler(const void* msg_ptr) {
    const auto* msg = static_cast<const LowState*>(msg_ptr);
    fmt::print("Received message:header-> {},{},{} | {},{},{} | {},{},{}\n", msg->imu_state().acc()[0],
               msg->imu_state().acc()[1], msg->imu_state().acc()[2], msg->imu_state().gyro()[0],
               msg->imu_state().gyro()[1], msg->imu_state().gyro()[2], msg->imu_state().rpy()[0],
               msg->imu_state().rpy()[1], msg->imu_state().rpy()[2]);

    return;
}

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto sub = std::make_unique<jrc::ChannelSubscriber<LowState>>(TOPIC, Handler);
    sub->InitChannel();
    for (size_t i = 0; i < 1000; ++i) {
        fmt::print("Sub | {} :Sleep 1 second\n", sub->GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    sub->CloseChannel();
    return 0;
}
