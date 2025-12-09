//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// DYNAMIC
#include "common/dds/dds_dynamic_factory.hpp"
// PUB
#include "robot/channel/channel_publisher.hpp"

constexpr auto TOPIC = "union_test";

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto sensor_type_builder =
        jcd::DdsDynamicFactory::ParserTypeFromIdlWithRos2("../../idl/SensorTest.idl", "test::Sensor", "../../idl/");
    auto sensor_type = sensor_type_builder->build();

    auto pub = std::make_unique<jrc::ChannelPublisher<jcd::DdsDynamicData>>(TOPIC, sensor_type_builder);
    pub->InitChannel();
    for (size_t i = 0; i < 1000; ++i) {
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
        auto msg = jcd::DdsDynamicFactory::ParseFromJson(j, sensor_type);
        pub->Write(&msg);

        fmt::print("Pub | {} | : Write message\n", pub->GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    pub->CloseChannel();
    return 0;
}
