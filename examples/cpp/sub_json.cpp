//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// DYNAMIC
#include "common/dds/dds_dynamic_factory.hpp"
// SUB
#include "robot/channel/channel_subscriber.hpp"

constexpr auto TOPIC = "rt/low_state";

namespace jrc = jsr::robot::channel;

namespace jcd = jsr::common::dds;

void SubHandle(const void* msg) {
    auto data = *static_cast<const jcd::DdsDynamicData::_ref_type*>(msg);
    auto json = jcd::DdsDynamicFactory::ToJson(data);
    fmt::print("Recv:{}\n", json.dump().c_str());
}

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto lowstate_type_builder =
        // jcd::DdsDynamicFactory::ParserTypeFromIdl("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
        jcd::DdsDynamicFactory::ParserTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");

    auto lowstate_type = lowstate_type_builder->build();
    fmt::print("{}", jcd::DdsDynamicFactory::IdlSerialize(lowstate_type).c_str());

    auto sub = std::make_unique<jrc::ChannelSubscriber<jcd::DdsDynamicData>>(TOPIC, lowstate_type_builder, SubHandle);

    sub->InitChannel();

    for (size_t i = 0; i < 1000; ++i) {
        fmt::print("Sub | {} :Sleep 1 second\n", sub->GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    sub->CloseChannel();

    return 0;
}