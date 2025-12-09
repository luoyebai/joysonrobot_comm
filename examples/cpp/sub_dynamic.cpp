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
    fmt::print(">>>>>>>>>>>>>>>>\n");
    auto data = *static_cast<const jcd::DdsDynamicData::_ref_type*>(msg);
    auto bms_state = data->loan_value(data->get_member_id_by_name("bms_state"));
    auto imu_state = data->loan_value(data->get_member_id_by_name("imu_state"));
    auto motor_state_parallel = data->loan_value(data->get_member_id_by_name("motor_state_parallel"));
    auto motor_state_serial = data->loan_value(data->get_member_id_by_name("motor_state_serial"));
    float voltage, current, remaining_cap, total_cap, soc;
    uint32_t cycle;
    bms_state->get_float32_value(voltage, bms_state->get_member_id_by_name("voltage"));
    bms_state->get_float32_value(current, bms_state->get_member_id_by_name("current"));
    bms_state->get_float32_value(remaining_cap, bms_state->get_member_id_by_name("remaining_cap"));
    bms_state->get_float32_value(total_cap, bms_state->get_member_id_by_name("total_cap"));
    bms_state->get_float32_value(soc, bms_state->get_member_id_by_name("soc"));
    bms_state->get_uint32_value(cycle, bms_state->get_member_id_by_name("cycle"));
    fmt::print("Bms:\n\tvoltage: {}, current: {}, remaining_cap: {}, total_cap: {}, soc: {}, cycle: {}\n", voltage,
               current, remaining_cap, total_cap, soc, cycle);

    std::vector<float> rpy, gyro, acc;
    imu_state->get_float32_values(rpy, imu_state->get_member_id_by_name("rpy"));
    imu_state->get_float32_values(gyro, imu_state->get_member_id_by_name("gyro"));
    imu_state->get_float32_values(acc, imu_state->get_member_id_by_name("acc"));
    assert(rpy.size() == 3);
    assert(gyro.size() == 3);
    assert(acc.size() == 3);
    fmt::print("Imu:\n\t");
    for (size_t i = 0; i < 3; ++i) {
        fmt::print("rpy[{0}]: {1}, gyro[{0}]: {2}, acc[{0}]: {3}\n\t", i, rpy[i], gyro[i], acc[i]);
    }

    auto motor_f = [](jcd::DdsDynamicData::_ref_type& motor_seq, const std::string& name) {
        fmt::print("{}:\n\t", name);
        for (size_t i = 0; i < 23; ++i) {
            auto motor_data = motor_seq->loan_value(i);
            uint8_t mode, temp;
            float q, dq, ddq, tau_est;
            uint32_t lost;
            std::vector<uint32_t> reserve;
            motor_data->get_uint8_value(mode, motor_data->get_member_id_by_name("mode"));
            motor_data->get_float32_value(q, motor_data->get_member_id_by_name("q"));
            motor_data->get_float32_value(dq, motor_data->get_member_id_by_name("dq"));
            motor_data->get_float32_value(ddq, motor_data->get_member_id_by_name("ddq"));
            motor_data->get_float32_value(tau_est, motor_data->get_member_id_by_name("tau_est"));
            motor_data->get_uint8_value(temp, motor_data->get_member_id_by_name("temperature"));
            motor_data->get_uint32_value(lost, motor_data->get_member_id_by_name("lost"));
            motor_data->get_uint32_values(reserve, motor_data->get_member_id_by_name("reserve"));
            fmt::print(
                "Motor[{}]:mode: {}, q: {}, dq: {}, ddq: {}, tau_est: {}, temp: {}, lost: {}, reserve[0]: {}, "
                "reserve[1]: {}\n\t",
                i, mode, q, dq, ddq, tau_est, temp, lost, reserve[0], reserve[1]);
            motor_seq->return_loaned_value(motor_data);
        }
    };

    motor_f(motor_state_parallel, "motors parallel");
    motor_f(motor_state_serial, "motors serial");

    data->return_loaned_value(imu_state);
    data->return_loaned_value(bms_state);
    data->return_loaned_value(motor_state_parallel);
    data->return_loaned_value(motor_state_serial);

    fmt::print("<<<<<<<<<<<<<<<<\n");
}

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto lowstate_type_builder =
        // jcd::DdsDynamicFactory::ParserTypeFromIdl("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
        jcd::DdsDynamicFactory::ParserTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");

    auto lowstate_type = lowstate_type_builder->build();
    jcd::DdsDynamicFactory::PrintTypeInfo(lowstate_type);

    auto sub = std::make_unique<jrc::ChannelSubscriber<jcd::DdsDynamicData>>(TOPIC, lowstate_type_builder, SubHandle);

    sub->InitChannel();

    for (size_t i = 0; i < 1000; ++i) {
        fmt::print("Sub | {} :Sleep 1 second\n", sub->GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    sub->CloseChannel();

    return 0;
}