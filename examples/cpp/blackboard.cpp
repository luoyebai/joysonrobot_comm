// STD
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
// FMT
#define FMT_HEADER_ONLY
#include <fmt/core.h>
// JSRCOMM
#include "jsrcomm/robot/channel/channel_blackboard.hpp"
// IDL
#include "jsrcomm/idl/LowState.hpp"

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

constexpr auto TOPIC_NAME = "rt/low_state";
constexpr auto SLEEP_TIME = 100;  // ms

int main() {
    jrc::ChannelFactory::instance()->init(0);
    jrc::ChannelBlackboard bb;
    bb.registerTopic<jsr::msg::LowState>(TOPIC_NAME);
    while (true) {
        const auto low_state = bb.get<jsr::msg::LowState>(TOPIC_NAME);
        const auto time = bb.get_timestamp<jsr::msg::LowState>(TOPIC_NAME);
        if (low_state.has_value()) {
            auto imu_state = low_state.value().imu_state();
            fmt::print("[{:.2f}] Got imu state:\n", time.value());
            fmt::print("\tacc = {} {} {}\n", imu_state.acc()[0], imu_state.acc()[1], imu_state.acc()[2]);
            fmt::print("\tgyro = {} {} {}\n", imu_state.gyro()[0], imu_state.gyro()[1], imu_state.gyro()[2]);
            fmt::print("\trpy = {} {} {}\n", imu_state.rpy()[0], imu_state.rpy()[1], imu_state.rpy()[2]);

            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        }
    }
    return 0;
}
