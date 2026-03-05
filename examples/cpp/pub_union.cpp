//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// DYNAMIC
#include "jsrcomm/common/dds/dds_dynamic_factory.hpp"
// PUB
#include "jsrcomm/robot/channel/channel_publisher.hpp"

constexpr auto TOPIC = "union_test";

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto sensor_type_builder =
        jcd::DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/SensorTest.idl", "test::Sensor", "../../idl/");
    auto sensor_type = sensor_type_builder->build();

    auto pub = std::make_unique<jrc::ChannelPublisher<jcd::DdsDynamicData>>(TOPIC, sensor_type_builder);
    pub->initChannel();

    constexpr size_t MSG_NUMS = 1000;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < MSG_NUMS; ++i) {
        auto j = nlohmann::json{};
        switch (i % 3) {
            case 0: {
                j = {{"id", "abcde"}, {"data", {{"temperature", 50}}}};
            } break;
            case 1: {
                j = {{"id", "abcde"}, {"data", {{"voltage", 48.0f}}}};
            } break;
            case 2: {
                j = {{"id", "abcde"}, {"data", {{"power", 200}}}};
            } break;
        }
        auto msg = jcd::DdsDynamicFactory::parseFromJson(j, sensor_type);
        pub->write(&msg);

        fmt::print("Pub | {} | : Write message\n", pub->getChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }
    pub->closeChannel();
    return 0;
}
