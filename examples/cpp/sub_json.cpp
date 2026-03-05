//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// DYNAMIC
#include "jsrcomm/common/dds/dds_dynamic_factory.hpp"
// SUB
#include "jsrcomm/robot/channel/channel_subscriber.hpp"

constexpr auto TOPIC = "rt/low_state";

namespace jrc = jsr::robot::channel;

namespace jcd = jsr::common::dds;

void SubHandle(const void* msg) {
    auto data = *static_cast<const jcd::DdsDynamicData::_ref_type*>(msg);
    auto json = jcd::DdsDynamicFactory::toJson(data);
    fmt::print("Recv:{}\n", json.dump().c_str());
}

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto lowstate_type_builder =
        // jcd::DdsDynamicFactory::parseTypeFromIdl("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
        jcd::DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");

    auto lowstate_type = lowstate_type_builder->build();
    fmt::print("{}", jcd::DdsDynamicFactory::idlSerialize(lowstate_type).c_str());

    auto sub = std::make_unique<jrc::ChannelSubscriber<jcd::DdsDynamicData>>(TOPIC, lowstate_type_builder, SubHandle);

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