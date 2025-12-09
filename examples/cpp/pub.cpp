//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// PUB
#include "idl/LowState.hpp"
#include "robot/channel/channel_publisher.hpp"

constexpr auto TOPIC = "rt/low_state";

namespace jrc = jsr::robot::channel;

using namespace jsr::msg;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto pub = std::make_unique<jrc::ChannelPublisher<LowState>>(TOPIC);
    pub->InitChannel();
    LowState msg;
    for (size_t i = 0; i < 1000; ++i) {
        pub->Write(&msg);
        fmt::print("Pub | {} | : Write message\n", pub->GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    pub->CloseChannel();
    return 0;
}
