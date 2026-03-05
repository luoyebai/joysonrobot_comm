//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// DYNAMIC
#include "jsrcomm/common/dds/dds_dynamic_factory.hpp"
// PUB
#include "jsrcomm/robot/channel/channel_publisher.hpp"

constexpr auto TOPIC = "rt/low_state";

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto lowstate_type_builder =
        jcd::DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    auto lowstate_type = lowstate_type_builder->build();
    auto pub = std::make_unique<jrc::ChannelPublisher<jcd::DdsDynamicData>>(TOPIC, lowstate_type_builder);
    pub->initChannel();

    auto motor_json = nlohmann::json{{"mode", 1},
                                     {"q", 0.1},
                                     {"dq", 0.01},
                                     {"ddq", 0.001},
                                     {"tau_est", 0.5},
                                     {"temperature", 30},
                                     {"lost", static_cast<uint32_t>(10)},
                                     {"reserve", {0, 0}}};
    auto data_json =
        nlohmann::json{{"bms_state",
                        {{"current", 1.0},
                         {"cycle", 10},
                         {"remaining_cap", 10.0},
                         {"soc", 2.0},
                         {"total_cap", 3.0},
                         {"voltage", 48.0}}},
                       {"imu_state", {{"acc", {1.0, 0.0, 0.0}}, {"gyro", {0.0, 1.0, 0.0}}, {"rpy", {0.0, 0.0, 1.0}}}},
                       {"motor_state_parallel", nlohmann::json::array()},
                       {"motor_state_serial", nlohmann::json::array()}};

    constexpr size_t MOTOR_NUM = 23;
    for (int i = 0; i < MOTOR_NUM; ++i) {
        data_json["motor_state_parallel"].push_back(motor_json);
        data_json["motor_state_serial"].push_back(motor_json);
    }

    constexpr size_t MSG_NUM = 1000;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < MSG_NUM; ++i) {
        auto data = jcd::DdsDynamicFactory::parseFromJson(data_json, lowstate_type);
        pub->write(&data);
        fmt::print("Pub | {} | : Write message\n", pub->getChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }

    pub->closeChannel();

    return 0;
}
