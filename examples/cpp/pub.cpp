//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// PUB
#include "jsrcomm/idl/LowState.hpp"
#include "jsrcomm/robot/channel/channel_publisher.hpp"

constexpr auto TOPIC = "rt/low_state";

namespace jrc = jsr::robot::channel;

using namespace jsr::msg;

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto pub = std::make_unique<jrc::ChannelPublisher<LowState>>(TOPIC);
    pub->initChannel();
    auto msg = LowState();
    constexpr size_t MSG_NUMS = 1000;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < MSG_NUMS; ++i) {
        pub->write(&msg);
        fmt::print("Pub | {} | : Write message\n", pub->getChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }
    pub->closeChannel();
    return 0;
}
