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
    using DyData = jcd::DdsDynamicData;

    jrc::ChannelFactory::instance()->init(0);

    auto lowstate_type_builder =
        jcd::DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    auto lowstate_type = lowstate_type_builder->build();
    jcd::DdsDynamicFactory::printTypeInfo(lowstate_type);

    auto pub = std::make_unique<jrc::ChannelPublisher<jcd::DdsDynamicData>>(TOPIC, lowstate_type_builder);
    pub->initChannel();

    auto data = jcd::DdsDynamicFactory::createData(lowstate_type);

    auto bms_state = data->loan_value(data->get_member_id_by_name("bms_state"));
    auto imu_state = data->loan_value(data->get_member_id_by_name("imu_state"));
    auto motor_state_parallel = data->loan_value(data->get_member_id_by_name("motor_state_parallel"));
    auto motor_state_serial = data->loan_value(data->get_member_id_by_name("motor_state_serial"));
    // bms
    bms_state->set_float32_value(bms_state->get_member_id_by_name("voltage"), 48.0f);
    bms_state->set_float32_value(bms_state->get_member_id_by_name("current"), 1.0f);
    bms_state->set_float32_value(bms_state->get_member_id_by_name("remaining_cap"), 6.0f);
    bms_state->set_float32_value(bms_state->get_member_id_by_name("total_cap"), 10.0f);
    bms_state->set_float32_value(bms_state->get_member_id_by_name("soc"), 0.6f);
    bms_state->set_uint32_value(bms_state->get_member_id_by_name("cycle"), 10);
    // imu
    imu_state->set_float32_values(imu_state->get_member_id_by_name("rpy"), {0.1f, 0.2f, 0.3f});
    imu_state->set_float32_values(imu_state->get_member_id_by_name("gyro"), {0.1f, 0.2f, 0.3f});
    imu_state->set_float32_values(imu_state->get_member_id_by_name("acc"), {0.1f, 0.2f, 9.8f});
    // motors
    constexpr size_t MOTORS_NUM = 23;
    for (size_t i = 0; i < MOTORS_NUM; ++i) {
        auto seq_data_p = motor_state_parallel->loan_value(i);
        auto seq_data_s = motor_state_serial->loan_value(i);
        static const auto mode_id = seq_data_p->get_member_id_by_name("mode");
        static const auto q_id = seq_data_p->get_member_id_by_name("q");
        static const auto dq_id = seq_data_p->get_member_id_by_name("dq");
        static const auto ddq_id = seq_data_p->get_member_id_by_name("ddq");
        static const auto tau_est_id = seq_data_p->get_member_id_by_name("tau_est");
        static const auto temp_id = seq_data_p->get_member_id_by_name("temperature");
        static const auto lost_id = seq_data_p->get_member_id_by_name("lost");
        static const auto reserve_id = seq_data_p->get_member_id_by_name("reserve");
        seq_data_p->set_uint8_value(mode_id, 1);
        seq_data_p->set_float32_value(q_id, 0.123f);
        seq_data_p->set_float32_value(dq_id, 0.234f);
        seq_data_p->set_float32_value(ddq_id, 0.345f);
        seq_data_p->set_float32_value(tau_est_id, 0.456f);
        seq_data_p->set_uint8_value(temp_id, 55);
        seq_data_p->set_uint32_value(lost_id, 0);
        seq_data_p->set_uint32_values(reserve_id, {0, 0});
        seq_data_s->set_uint8_value(mode_id, 2);
        seq_data_s->set_float32_value(q_id, 1.123f);
        seq_data_s->set_float32_value(dq_id, 1.234f);
        seq_data_s->set_float32_value(ddq_id, 1.345f);
        seq_data_s->set_float32_value(tau_est_id, 1.456f);
        seq_data_s->set_uint8_value(temp_id, 56);
        seq_data_s->set_uint32_value(lost_id, 1);
        seq_data_s->set_uint32_values(reserve_id, {1, 1});
        motor_state_parallel->return_loaned_value(seq_data_p);
        motor_state_serial->return_loaned_value(seq_data_s);
    }

    // pub 1000 messages
    constexpr size_t MSG_NUM = 1000;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < MSG_NUM; ++i) {
        pub->write(&data);
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }

    data->return_loaned_value(bms_state);
    data->return_loaned_value(imu_state);
    data->return_loaned_value(motor_state_parallel);
    data->return_loaned_value(motor_state_serial);
    pub->closeChannel();

    return 0;
}
