//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// IDL
#include "jsrcomm/idl/SensorTest.hpp"
// SUB
#include "jsrcomm/robot/channel/channel_subscriber.hpp"

constexpr auto TOPIC = "union_test";

namespace jrc = jsr::robot::channel;

void Handler(const void* msg_ptr) {
    const auto* msg = static_cast<const test::Sensor*>(msg_ptr);
    fmt::print("Sub Received id:{}\n", msg->id());
    switch (msg->data()._d()) {
        case 0: {
            fmt::print("Sub Received message temperature:{}\n", msg->data().temperature());
        } break;
        case 1: {
            fmt::print("Sub Received message voltage:{}\n", msg->data().voltage());
        } break;
        case 2: {
            fmt::print("Sub Received message power:{}\n", msg->data().power());
        } break;
        default: {
            fmt::print("Sub Received message unknown\n");
        } break;
    }
    return;
}

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto sub = std::make_unique<jrc::ChannelSubscriber<test::Sensor>>(TOPIC, Handler);
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
