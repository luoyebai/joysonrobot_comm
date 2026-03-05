//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// IDL
#include "jsrcomm/idl/LowState.hpp"
// SUB
#include "jsrcomm/robot/channel/channel_subscriber.hpp"

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
    jrc::ChannelFactory::instance()->init(0);
    auto sub = std::make_unique<jrc::ChannelSubscriber<LowState>>(TOPIC, Handler);
    sub->initChannel();
    constexpr size_t SLEEP_COUNT = 1000;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < SLEEP_COUNT; ++i) {
        fmt::print("Sub | {} :Sleep 1 second\n", sub->getChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }
    sub->closeChannel();
    return 0;
}
