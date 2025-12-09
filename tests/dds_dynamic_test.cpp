#define CATCH_CONFIG_MAIN
// STD
#include <thread>
// TEST
#include "test_tools/catch_amalgamated.hpp"
// DDS TYPE
#include "common/dds/dds_dynamic_factory.hpp"
#include "robot/channel/channel_publisher.hpp"
#include "robot/channel/channel_subscriber.hpp"

namespace jrc = jsr::robot::channel;

TEST_CASE("Publisher/Subscriber Dynamic pub&sub DDS communication using dynamic data test cases", "[PUBSUB_DY]") {
    using namespace jsr::common::dds;
    jrc::ChannelFactory::Instance()->Init(0);

    auto member1_builder = DdsDynamicFactory::CreateStructType("Member1");
    DdsDynamicFactory::AddPrimitiveMember(member1_builder, "first", eprosima::fastdds::dds::TK_INT32, 16);
    DdsDynamicFactory::AddPrimitiveMember(member1_builder, "second", eprosima::fastdds::dds::TK_INT32, 18);
    auto member1_type = member1_builder->build();

    auto member2_builder = DdsDynamicFactory::CreateStructType("Member2");
    DdsDynamicFactory::AddPrimitiveMember(member2_builder, "first", TK_FLOAT32);
    DdsDynamicFactory::AddPrimitiveMember(member2_builder, "second", TK_INT64);
    DdsDynamicFactory::AddSequenceMember(member2_builder, "seq", member1_type);
    auto member2_type = member2_builder->build();

    auto complex_builder = DdsDynamicFactory::CreateStructType("ComplexStruct");
    DdsDynamicFactory::AddCustomMember(complex_builder, "member1", member1_type);
    DdsDynamicFactory::AddCustomMember(complex_builder, "member2", member2_type);
    auto complex_type = complex_builder->build();

    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>("ComplexTest", complex_builder);
    auto sub = std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>(
        "ComplexTest", complex_builder, [complex_type](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);
            DdsDynamicData::_ref_type m1_loan_data = data->loan_value(data->get_member_id_by_name("member1"));
            DdsDynamicData::_ref_type m2_loan_data = data->loan_value(data->get_member_id_by_name("member2"));
            int32_t m1_first, m1_second;
            float m2_first;
            int64_t m2_second;
            DdsDynamicData::_ref_type m2_seq;
            m1_loan_data->get_int32_value(m1_first, m1_loan_data->get_member_id_by_name("first"));
            m1_loan_data->get_int32_value(m1_second, m1_loan_data->get_member_id_by_name("second"));
            data->return_loaned_value(m1_loan_data);
            m2_loan_data->get_float32_value(m2_first, m2_loan_data->get_member_id_by_name("first"));
            m2_loan_data->get_int64_value(m2_second, m2_loan_data->get_member_id_by_name("second"));
            m2_loan_data->get_complex_value(m2_seq, m2_loan_data->get_member_id_by_name("seq"));

            data->return_loaned_value(m2_loan_data);

            for (size_t i = 0; i < 100; ++i) {
                int32_t first, second;
                DdsDynamicData::_ref_type m2_seq_data = m2_seq->loan_value(i);
                m2_seq_data->get_int32_value(first, m1_loan_data->get_member_id_by_name("first"));
                m2_seq_data->get_int32_value(second, m1_loan_data->get_member_id_by_name("second"));
                m2_seq->return_loaned_value(m2_seq_data);
                REQUIRE(first == 12315 + i * 10);
                REQUIRE(second == 12306 + i * 10);
            }

            REQUIRE(m1_first == 12315);
            REQUIRE(m1_second == 12306);
            REQUIRE(m2_first == Catch::Approx(666.f).epsilon(1e-5));
            REQUIRE(m2_second == 0xFFFFF);
            return;
        });
    pub->InitChannel();
    sub->InitChannel();

    auto data = DdsDynamicFactory::CreateData(complex_type);

    BENCHMARK("Publisher Dynamic DDS 100 count communication benchmark") {
        for (size_t i = 0; i < 100; i++) {
            DdsDynamicData::_ref_type member1_data;
            data->get_complex_value(member1_data, data->get_member_id_by_name("member1"));
            member1_data->set_int32_value(member1_data->get_member_id_by_name("first"), 12315);
            member1_data->set_int32_value(member1_data->get_member_id_by_name("second"), 12306);
            data->set_complex_value(data->get_member_id_by_name("member1"), member1_data);

            DdsDynamicData::_ref_type member2_data;
            data->get_complex_value(member2_data, data->get_member_id_by_name("member2"));
            member2_data->set_float32_value(member2_data->get_member_id_by_name("first"), 666.f);
            member2_data->set_int64_value(member2_data->get_member_id_by_name("second"), 0xFFFFF);

            DdsDynamicData::_ref_type m2_seq = member2_data->loan_value(member2_data->get_member_id_by_name("seq"));
            for (size_t j = 0; j < 100; ++j) {
                DdsDynamicData::_ref_type seq_data = member1_data;
                seq_data->set_int32_value(seq_data->get_member_id_by_name("first"), 12315 + j * 10);
                seq_data->set_int32_value(seq_data->get_member_id_by_name("second"), 12306 + j * 10);
                m2_seq->set_complex_value(j, seq_data);
            }
            member2_data->return_loaned_value(m2_seq);

            data->set_complex_value(data->get_member_id_by_name("member2"), member2_data);

            pub->Write(&data);
        }
    };

    pub->CloseChannel();
    sub->CloseChannel();
}

TEST_CASE("Publisher/Subscriber Dynamic pub& Static sub DDS communication using dynamic data test cases",
          "[PUBSUB_DY_USE_IDL]") {
    using namespace jsr::common::dds;
    constexpr auto TOPIC_NAME = "rt/low_state";
    // lowstate
    jrc::ChannelFactory::Instance()->Init(0);

    auto lowstate_type_builder =
        DdsDynamicFactory::ParserTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    auto lowstate_type = lowstate_type_builder->build();
    DdsDynamicFactory::PrintTypeInfo(lowstate_type);
    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>(TOPIC_NAME, lowstate_type_builder);
    auto sub = std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>(
        TOPIC_NAME, lowstate_type_builder, [](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);
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
            REQUIRE(voltage == Catch::Approx(48.0f).epsilon(1e-5));
            REQUIRE(current == Catch::Approx(1.0f).epsilon(1e-5));
            REQUIRE(remaining_cap == Catch::Approx(6.0f).epsilon(1e-5));
            REQUIRE(total_cap == Catch::Approx(10.0f).epsilon(1e-5));
            REQUIRE(soc == Catch::Approx(0.6f).epsilon(1e-5));
            REQUIRE(cycle == 10);

            std::vector<float> rpy, gyro, acc;
            imu_state->get_float32_values(rpy, imu_state->get_member_id_by_name("rpy"));
            imu_state->get_float32_values(gyro, imu_state->get_member_id_by_name("gyro"));
            imu_state->get_float32_values(acc, imu_state->get_member_id_by_name("acc"));
            REQUIRE(rpy.size() == 3);
            REQUIRE(gyro.size() == 3);
            REQUIRE(acc.size() == 3);
            for (size_t i = 0; i < 3; ++i) {
                REQUIRE(rpy[i] == Catch::Approx(0.1f * (i + 1)).epsilon(1e-5));
                REQUIRE(gyro[i] == Catch::Approx(0.1f * (i + 1)).epsilon(1e-5));
                if (i != 2) {
                    REQUIRE(acc[i] == Catch::Approx(0.1f * (i + 1)).epsilon(1e-5));
                } else {
                    REQUIRE(acc[i] == Catch::Approx(9.8f).epsilon(1e-5));
                }
            }

            auto motor_test = [](DdsDynamicData::_ref_type& motor_seq, size_t count) {
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
                    REQUIRE(mode == 1 + count);
                    REQUIRE(temp == 55 + count);
                    REQUIRE(q == Catch::Approx(count + 0.123f).epsilon(1e-5));
                    REQUIRE(dq == Catch::Approx(count + 0.234f).epsilon(1e-5));
                    REQUIRE(ddq == Catch::Approx(count + 0.345f).epsilon(1e-5));
                    REQUIRE(tau_est == Catch::Approx(count + 0.456f).epsilon(1e-5));
                    REQUIRE(lost == 0 + count);
                    REQUIRE(reserve.size() == 2);
                    for (size_t i = 0; i < 2; ++i) {
                        REQUIRE(reserve[i] == count + 0);
                    }
                    motor_seq->return_loaned_value(motor_data);
                }
            };
            motor_test(motor_state_parallel, 0);
            motor_test(motor_state_serial, 1);

            data->return_loaned_value(imu_state);
            data->return_loaned_value(bms_state);
            data->return_loaned_value(motor_state_parallel);
            data->return_loaned_value(motor_state_serial);
        });

    pub->InitChannel();
    sub->InitChannel();

    auto data = DdsDynamicFactory::CreateData(lowstate_type);

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
    for (size_t i = 0; i < 100; ++i) {
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

    BENCHMARK("Publisher Dynamic DDS LowState communication benchmark") {
        pub->Write(&data);
    };

    data->return_loaned_value(bms_state);
    data->return_loaned_value(imu_state);
    data->return_loaned_value(motor_state_parallel);
    data->return_loaned_value(motor_state_serial);

    pub->CloseChannel();
    sub->CloseChannel();
}

TEST_CASE("Idl Dynamic type union test", "[UNION]") {
    using namespace jsr::common::dds;

    union UnionVal {
        int8_t temperature;
        float voltage;
        int32_t power;
    };

    auto union_type_builder =
        DdsDynamicFactory::ParserTypeFromIdlWithRos2("../../idl/SensorTest.idl", "test::Sensor", "../../idl/");
    auto union_type = union_type_builder->build();
    DdsDynamicFactory::PrintTypeInfo(union_type);
    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>("union_test", union_type_builder);
    auto sub =
        std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>("union_test", union_type_builder, [](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);
            DdsDynamicData::_ref_type union_data;
            std::string id;
            data->get_string_value(id, data->get_member_id_by_name("id"));
            REQUIRE(id == "0177");
            data->get_complex_value(union_data, data->get_member_id_by_name("data"));
            uint8_t disc;
            UnionVal val;
            union_data->get_uint8_value(disc, union_data->get_member_id_by_name("discriminator"));
            switch (disc) {
                case 0:
                    union_data->get_int8_value(val.temperature, union_data->get_member_id_by_name("temperature"));
                    REQUIRE(val.temperature == 50);
                    break;
                case 1:
                    union_data->get_float32_value(val.voltage, union_data->get_member_id_by_name("voltage"));
                    REQUIRE(val.voltage == Catch::Approx(48.0));
                    break;
                case 2:
                    union_data->get_int32_value(val.power, union_data->get_member_id_by_name("power"));
                    REQUIRE(val.power == 200);
                    break;
            }
        });
    pub->InitChannel();
    sub->InitChannel();
    auto data = DdsDynamicFactory::CreateData(union_type);
    data->set_string_value(data->get_member_id_by_name("id"), "0177");
    auto union_data = data->loan_value(data->get_member_id_by_name("data"));
    for (size_t i = 0; i < 1000; ++i) {
        switch (i % 3) {
            case 0:
                union_data->set_int8_value(union_data->get_member_id_by_name("temperature"), 50);
                break;
            case 1:
                union_data->set_float32_value(union_data->get_member_id_by_name("voltage"), 48.0f);
                break;
            case 2:
                union_data->set_int32_value(union_data->get_member_id_by_name("power"), 200);
                break;
        }
        pub->Write(&data);
    }
    data->return_loaned_value(union_data);

    pub->CloseChannel();
    sub->CloseChannel();
}

TEST_CASE("Idl Dynamic type serialize test", "[DY_JSON]") {
    using namespace jsr::common::dds;
    auto lowstate_type_builder =
        DdsDynamicFactory::ParserTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    auto lowstate_type = lowstate_type_builder->build();

    auto type_str = DdsDynamicFactory::IdlSerialize(lowstate_type);
    fmt::print("LowState type:\n{}", type_str.c_str());

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

    auto data = DdsDynamicFactory::ParseFromJson(data_json, lowstate_type);
    auto json = DdsDynamicFactory::ToJson(data);
    REQUIRE(json["bms_state"]["current"] == Catch::Approx(1.0));
    REQUIRE(json["bms_state"]["cycle"] == 10);
    REQUIRE(json["bms_state"]["remaining_cap"] == Catch::Approx(10.0));
    REQUIRE(json["bms_state"]["soc"] == Catch::Approx(2.0));
    REQUIRE(json["bms_state"]["total_cap"] == Catch::Approx(3.0));
    REQUIRE(json["bms_state"]["voltage"] == Catch::Approx(48.0));
    REQUIRE(json["imu_state"]["rpy"][0] == Catch::Approx(0.0));
    REQUIRE(json["imu_state"]["rpy"][1] == Catch::Approx(0.0));
    REQUIRE(json["imu_state"]["rpy"][2] == Catch::Approx(1.0));
    REQUIRE(json["imu_state"]["gyro"][0] == Catch::Approx(0.0));
    REQUIRE(json["imu_state"]["gyro"][1] == Catch::Approx(1.0));
    REQUIRE(json["imu_state"]["gyro"][2] == Catch::Approx(0.0));
    REQUIRE(json["imu_state"]["acc"][0] == Catch::Approx(1.0));
    REQUIRE(json["imu_state"]["acc"][1] == Catch::Approx(0.0));
    REQUIRE(json["imu_state"]["acc"][2] == Catch::Approx(0.0));
    REQUIRE(json["motor_state_parallel"].size() == 0);
    REQUIRE(json["motor_state_serial"].size() == 0);
}