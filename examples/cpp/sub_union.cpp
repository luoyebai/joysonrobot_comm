//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// IDL
#include "idl/SensorTest.hpp"
// SUB
#include "robot/channel/channel_subscriber.hpp"

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
    jrc::ChannelFactory::Instance()->Init(0);
    auto sub = std::make_unique<jrc::ChannelSubscriber<test::Sensor>>(TOPIC, Handler);
    sub->InitChannel();
    for (size_t i = 0; i < 1000; ++i) {
        fmt::print("Sub | {} :Sleep 1 second\n", sub->GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    sub->CloseChannel();
    return 0;
}
