// STD
#include <random>
#include <string>
#include <vector>
// FMT
#define FMT_HEADER_ONLY
#include <fmt/core.h>
// TEST
#include "test_tools/catch_amalgamated.hpp"
// DYNAMIC DDS
#include "jsrcomm/common/dds/dds_dynamic_factory.hpp"
// ROBOT CHANNEL
#include "jsrcomm/robot/channel/channel_publisher.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"
// RPC
#include "jsrcomm/robot/rpc/error.hpp"
#include "jsrcomm/robot/rpc/request.hpp"
#include "jsrcomm/robot/rpc/response.hpp"
// RPC CLIENT/SERVER
#include "jsrcomm/robot/rpc/rpc_client.hpp"
#include "jsrcomm/robot/rpc/rpc_server.hpp"
//IDL
#include "jsrcomm/idl/ImuState.hpp"
// GRPC
#include "jsrcomm/common/grpc/grpc_wrapper.hpp"
// PROTO
#include "jsrcomm/proto/hello_world.grpc.pb.h"

namespace jsr_test_fmt {
TEST_CASE("fmt basic formatting", "[FMT]") {
    REQUIRE(fmt::format("hello {}!", "world") == "hello world!");
    REQUIRE(fmt::format("{} + {} = {}", 1, 2, 3) == "1 + 2 = 3");
    REQUIRE(fmt::format("{:.2f}", M_PI) == "3.14");
    REQUIRE(fmt::format("{:06d}", 42) == "000042");
}
}  // namespace jsr_test_fmt

namespace jsr_test_dds {
constexpr auto LR_LOCO_TOPIC = "lr/loco_ctrl";
constexpr auto LOCO_RPC_NAME = "loco";
namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;
using namespace jsr::msg;

constexpr size_t TEST_COUNT = 10000;
constexpr int64_t MAX_API_ID = 5000;

TEST_CASE("Publisher/Subscriber DDS communication test cases", "[DDS]") {
    // Call once
    jrc::ChannelFactory::instance()->init(0);
    auto g_f = [&]() {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return static_cast<float>(dist(rng));
    };

    auto pubmsg = ImuState();
    jrc::ChannelPublisher<ImuState> pub(LR_LOCO_TOPIC);
    jrc::ChannelSubscriber<ImuState> sub(LR_LOCO_TOPIC, [&pubmsg](const void* msg_ptr) {
        const auto* msg = static_cast<const ImuState*>(msg_ptr);
        REQUIRE(*msg == pubmsg);
    });

    pub.initChannel();
    sub.initChannel();
    for (size_t i = 0; i < TEST_COUNT; ++i) {
        pubmsg.gyro({g_f(), g_f(), g_f()});
        pubmsg.acc({g_f(), g_f(), g_f()});
        pubmsg.rpy({g_f(), g_f(), g_f()});
        pub.write(&pubmsg);
    }

    pub.closeChannel();
    sub.closeChannel();
}

class LocoServer : public jrr::RpcServer {
   public:
    LocoServer() = default;
    ~LocoServer() = default;
    jrr::Response HandleRequest(jrr::Request& request) override {
        auto response = jrr::Response();
        response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
        response.SetBody(request.MoveBody());
        return response;
    }
};

bool IsRequestOk(const jrr::Request& req, const jrr::Response& resp) {
    // No error handling in server
    return (resp.GetHeader().GetStatus() == jrr::RPC_STATUS_CODE_SUCCESS && req.GetBody() == resp.GetBody());
}

TEST_CASE("Rpc Client/Server communication test cases", "[DDS][RPC]") {
    auto api_id = static_cast<int64_t>(random() % MAX_API_ID);
    auto static server = std::make_shared<LocoServer>();
    auto static client = std::make_shared<jrr::RpcClient>();
    server->init(LOCO_RPC_NAME);
    client->init(LOCO_RPC_NAME);
    auto req = jrr::Request(jrr::RequestHeader(api_id), std::string(8000, 'X'));
    for (size_t i = 0; i < TEST_COUNT; ++i) {
        auto resp = client->SendApiRequest(req);
        REQUIRE(IsRequestOk(req, resp));
    }
    client->Stop();
    server->Stop();
}

/**
 * @brief Test DDS communication MockServer, Don't use Stop function in the HandleRequest function
 * You should use atomic_bool to stop the server
 *
 */
class MockServer : public jrr::RpcServer {
   public:
    std::atomic_flag stopped = ATOMIC_FLAG_INIT;

   private:
    jrr::Response HandleRequest(jrr::Request& request) override {
        auto api_id = request.GetHeader().GetApiId();
        auto response = jrr::Response();
        switch (api_id) {
            case 0:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                stopped.test_and_set();
                break;
            default:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SERVER_REFUSED));
                break;
        }
        return response;
    }
};

TEST_CASE("Rpc Client/Server communication test cases with timeout", "[DDS][RPC]") {
    auto static server = std::make_unique<MockServer>();
    auto static client = std::make_unique<jrr::RpcClient>();
    server->init("TimeoutTest");
    client->init("TimeoutTest");
    int64_t status = -1;
    auto req = jrr::Request(jrr::RequestHeader(0), "");
    status = client->SendApiRequest(req).GetHeader().GetStatus();
    REQUIRE(status == jrr::RPC_STATUS_CODE_SUCCESS);
    if (server->stopped._M_i) {
        server->Stop();
    }
    status = client->SendApiRequest(req).GetHeader().GetStatus();
    REQUIRE(status == jrr::RPC_STATUS_CODE_TIMEOUT);
    client->Stop();
}

TEST_CASE("Rpc Client/Server communication aysnc test cases", "[DDS][RPC][ASYNC]") {
    auto api_id = static_cast<int64_t>(random() % MAX_API_ID);
    auto static server = std::make_shared<LocoServer>();
    auto static client = std::make_shared<jrr::RpcClient>();
    server->init(LOCO_RPC_NAME);
    client->init(LOCO_RPC_NAME);
    auto req = jrr::Request(jrr::RequestHeader(api_id), std::string(8000, 'X'));

    std::atomic<size_t> done_count{0};
    for (size_t i = 0; i < TEST_COUNT; ++i) {
        client->SendApiRequestAsync(req, [&](const jrr::Response& resp) {
            REQUIRE(IsRequestOk(req, resp));
            done_count.fetch_add(1, std::memory_order_relaxed);
            return;
        });
    }

    const auto start = std::chrono::steady_clock::now();
    while (done_count.load() < TEST_COUNT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            FAIL("Async RPC test timed out. Not all callbacks finished.");
            break;
        }
    }
}

}  // namespace jsr_test_dds

namespace jsr_test_dds_dynamic {
namespace jrc = jsr::robot::channel;

constexpr auto TEST_COUNT = 100;
constexpr auto CAP_SIZE = 100;

TEST_CASE("Publisher/Subscriber Dynamic pub&sub DDS communication using dynamic data test cases", "[DDS][DYNAMIC]") {
    using namespace jsr::common::dds;

    auto member1_builder = DdsDynamicFactory::createStructType("Member1");
    DdsDynamicFactory::addPrimitiveMember(member1_builder, "first", eprosima::fastdds::dds::TK_INT32, 16);
    DdsDynamicFactory::addPrimitiveMember(member1_builder, "second", eprosima::fastdds::dds::TK_INT32, 18);
    auto member1_type = member1_builder->build();

    auto member2_builder = DdsDynamicFactory::createStructType("Member2");
    DdsDynamicFactory::addPrimitiveMember(member2_builder, "first", TK_FLOAT32);
    DdsDynamicFactory::addPrimitiveMember(member2_builder, "second", TK_INT64);
    DdsDynamicFactory::addSequenceMember(member2_builder, "seq", member1_type);
    auto member2_type = member2_builder->build();

    auto complex_builder = DdsDynamicFactory::createStructType("ComplexStruct");
    DdsDynamicFactory::addCustomMember(complex_builder, "member1", member1_type);
    DdsDynamicFactory::addCustomMember(complex_builder, "member2", member2_type);
    auto complex_type = complex_builder->build();

    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>("ComplexTest", complex_builder);
    auto sub = std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>(
        "ComplexTest", complex_builder, [complex_type](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);
            DdsDynamicData::_ref_type m1_loan_data = data->loan_value(data->get_member_id_by_name("member1"));
            DdsDynamicData::_ref_type m2_loan_data = data->loan_value(data->get_member_id_by_name("member2"));
            int32_t m1_first = 0, m1_second = 0;
            float m2_first = 0.0f;
            int64_t m2_second = 0;
            DdsDynamicData::_ref_type m2_seq;
            m1_loan_data->get_int32_value(m1_first, m1_loan_data->get_member_id_by_name("first"));
            m1_loan_data->get_int32_value(m1_second, m1_loan_data->get_member_id_by_name("second"));
            data->return_loaned_value(m1_loan_data);
            m2_loan_data->get_float32_value(m2_first, m2_loan_data->get_member_id_by_name("first"));
            m2_loan_data->get_int64_value(m2_second, m2_loan_data->get_member_id_by_name("second"));
            m2_loan_data->get_complex_value(m2_seq, m2_loan_data->get_member_id_by_name("seq"));

            data->return_loaned_value(m2_loan_data);

            for (size_t i = 0; i < CAP_SIZE; ++i) {
                int32_t first = 0, second = 0;
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
    pub->initChannel();
    sub->initChannel();

    auto data = DdsDynamicFactory::createData(complex_type);

    for (size_t i = 0; i < TEST_COUNT; i++) {
        auto member1_data = DdsDynamicData::_ref_type(nullptr);
        data->get_complex_value(member1_data, data->get_member_id_by_name("member1"));
        member1_data->set_int32_value(member1_data->get_member_id_by_name("first"), 12315);
        member1_data->set_int32_value(member1_data->get_member_id_by_name("second"), 12306);
        data->set_complex_value(data->get_member_id_by_name("member1"), member1_data);

        auto member2_data = DdsDynamicData::_ref_type(nullptr);
        data->get_complex_value(member2_data, data->get_member_id_by_name("member2"));
        member2_data->set_float32_value(member2_data->get_member_id_by_name("first"), 666.f);
        member2_data->set_int64_value(member2_data->get_member_id_by_name("second"), 0xFFFFF);

        DdsDynamicData::_ref_type m2_seq = member2_data->loan_value(member2_data->get_member_id_by_name("seq"));
        for (int32_t j = 0; j < CAP_SIZE; ++j) {
            const DdsDynamicData::_ref_type& seq_data = member1_data;
            seq_data->set_int32_value(seq_data->get_member_id_by_name("first"), 12315 + j * 10);
            seq_data->set_int32_value(seq_data->get_member_id_by_name("second"), 12306 + j * 10);
            m2_seq->set_complex_value(j, seq_data);
        }
        member2_data->return_loaned_value(m2_seq);
        data->set_complex_value(data->get_member_id_by_name("member2"), member2_data);
        pub->write(&data);
    }

    pub->closeChannel();
    sub->closeChannel();
}

constexpr auto TOPIC_NAME = "rt/low_state";
TEST_CASE("Publisher/Subscriber Dynamic pub& Static sub DDS communication using dynamic data test cases",
          "[DDS][DYNAMIC][IDL]") {
    using namespace jsr::common::dds;

    auto lowstate_type_builder =
        DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    auto lowstate_type = lowstate_type_builder->build();

    // DdsDynamicFactory::printTypeInfo(lowstate_type);

    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>(TOPIC_NAME, lowstate_type_builder);
    auto sub = std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>(
        TOPIC_NAME, lowstate_type_builder, [](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);
            auto bms_state = data->loan_value(data->get_member_id_by_name("bms_state"));
            auto imu_state = data->loan_value(data->get_member_id_by_name("imu_state"));
            auto motor_state_parallel = data->loan_value(data->get_member_id_by_name("motor_state_parallel"));
            auto motor_state_serial = data->loan_value(data->get_member_id_by_name("motor_state_serial"));

            float voltage{}, current{}, remaining_cap{}, total_cap{}, soc{};
            uint32_t cycle = 0;
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
                    uint8_t mode = 0, temp = 0;
                    float q{}, dq{}, ddq{}, tau_est{};
                    uint32_t lost = 0;
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

    pub->initChannel();
    sub->initChannel();

    auto data = DdsDynamicFactory::createData(lowstate_type);

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
    for (size_t i = 0; i < CAP_SIZE; ++i) {
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

    for (size_t i = 0; i < TEST_COUNT; ++i) {
        pub->write(&data);
    }

    data->return_loaned_value(bms_state);
    data->return_loaned_value(imu_state);
    data->return_loaned_value(motor_state_parallel);
    data->return_loaned_value(motor_state_serial);

    pub->closeChannel();
    sub->closeChannel();
}

TEST_CASE("Idl Dynamic type union test", "[DDS][DYNAMIC][UNION]") {
    using namespace jsr::common::dds;

    union UnionVal {
        int8_t temperature;
        float voltage;
        int32_t power;
    };

    auto union_type_builder =
        DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/SensorTest.idl", "test::Sensor", "../../idl/");
    auto union_type = union_type_builder->build();

    // DdsDynamicFactory::printTypeInfo(union_type);

    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>("union_test", union_type_builder);
    auto sub =
        std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>("union_test", union_type_builder, [](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);
            DdsDynamicData::_ref_type union_data;
            std::string id;
            data->get_string_value(id, data->get_member_id_by_name("id"));
            REQUIRE(id == "0177");
            data->get_complex_value(union_data, data->get_member_id_by_name("data"));
            uint8_t disc = 0;
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
    pub->initChannel();
    sub->initChannel();
    auto data = DdsDynamicFactory::createData(union_type);
    data->set_string_value(data->get_member_id_by_name("id"), "0177");
    auto union_data = data->loan_value(data->get_member_id_by_name("data"));
    for (size_t i = 0; i < TEST_COUNT; ++i) {
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
        pub->write(&data);
    }
    data->return_loaned_value(union_data);

    pub->closeChannel();
    sub->closeChannel();
}

TEST_CASE("Idl Dynamic type serialize test", "[DDS][DYNAMIC][JSON]") {
    using namespace jsr::common::dds;
    auto lowstate_type_builder =
        DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    auto lowstate_type = lowstate_type_builder->build();

    // auto type_str = DdsDynamicFactory::idlSerialize(lowstate_type);
    // fmt::print("LowState type:\n{}", type_str.c_str());

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

    auto data = DdsDynamicFactory::parseFromJson(data_json, lowstate_type);
    auto json = DdsDynamicFactory::toJson(data);
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
    REQUIRE(json["motor_state_parallel"].empty());
    REQUIRE(json["motor_state_serial"].empty());
}

}  // namespace jsr_test_dds_dynamic

namespace jsr_test_grpc {

// proto
using hello::Greeter1;
using hello::Greeter2;
using hello::HelloReply;
using hello::HelloRequest;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerReaderWriter;

template <class Greeter>
class GreeterClient {
   public:
    using ServiceObject = Greeter;
    using Wrapper = jsr::rpc::ClientWrapper<ServiceObject, HelloRequest, HelloReply>;
    explicit GreeterClient(std::shared_ptr<Channel> channel) : wrapper_(std::make_unique<Wrapper>(channel)) {}
    GreeterClient(const GreeterClient&) = delete;
    GreeterClient& operator=(const GreeterClient&) = delete;
    virtual ~GreeterClient() = default;

    std::string sayHello(const std::string& user) {
        auto request = HelloRequest{};
        request.set_name(user);
        return wrapper_->rpcCall(request, &ServiceObject::Stub::SayHello).message();
    }

    std::string sayHelloAgain(const std::string& user) {
        auto request = HelloRequest{};
        request.set_name(user);
        return wrapper_->rpcCall(request, &ServiceObject::Stub::SayHelloAgain).message();
    }

    void startChat(std::string& name) {
        chat_flag_.store(true);
        wrapper_->streamCall(
            &ServiceObject::Stub::Chat, chat_flag_,
            [&name] {
                HelloRequest req;
                req.set_name(name);
                return req;
            },
            [this](HelloReply* reply) {
                std::lock_guard lock(stream_mutex_);
                stream_messages_.push_back(reply->message());
            });
        return;
    }

    void runChat(const std::vector<std::string>& names) {
        size_t i = 0;
        wrapper_->streamCall(
            &ServiceObject::Stub::Chat, names.size(),
            [&names, &i] {
                HelloRequest req;
                req.set_name(names[i]);
                ++i;
                return req;
            },
            [this](HelloReply* reply) {
                std::lock_guard lock(stream_mutex_);
                stream_messages_.push_back(reply->message());
            });
        return;
    }

    std::vector<std::string> getStreamMessages() {
        std::lock_guard lock(stream_mutex_);
        return stream_messages_;
    }

    void stopChat() {
        chat_flag_.store(false);
        return;
    }

   private:
    std::unique_ptr<Wrapper> wrapper_{nullptr};
    std::atomic_bool chat_flag_{true};
    std::mutex stream_mutex_{};
    std::vector<std::string> stream_messages_{};
};

using grpc::ServerContext;
using grpc::Status;

template <class Greeter>
class GreeterServiceImpl final : public Greeter::Service {
    Status SayHello(ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
        auto user_name = request->name();

        static_assert(std::is_same_v<Greeter, Greeter1> || std::is_same_v<Greeter, Greeter2>);
        if constexpr (std::is_same_v<Greeter, Greeter1>) {
            reply->set_message("[1] Hello " + user_name);
        } else if constexpr (std::is_same_v<Greeter, Greeter2>) {
            reply->set_message("[2] Hello " + user_name);
        }

        return Status::OK;
    }
    Status SayHelloAgain(ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
        auto user_name = request->name();

        static_assert(std::is_same_v<Greeter, Greeter1> || std::is_same_v<Greeter, Greeter2>);
        if constexpr (std::is_same_v<Greeter, Greeter1>) {
            reply->set_message("[1] Hello again " + user_name);
        } else if constexpr (std::is_same_v<Greeter, Greeter2>) {
            reply->set_message("[2] Hello again " + user_name);
        }

        return Status::OK;
    }
    Status Chat(ServerContext* context, ServerReaderWriter<HelloReply, HelloRequest>* stream) override {
        HelloRequest req;
        while (stream->Read(&req)) {
            HelloReply reply;

            static_assert(std::is_same_v<Greeter, Greeter1> || std::is_same_v<Greeter, Greeter2>);

            if constexpr (std::is_same_v<Greeter, Greeter1>) {
                reply.set_message("[1] Stream Hello " + req.name());
            } else if constexpr (std::is_same_v<Greeter, Greeter2>) {
                reply.set_message("[2] Stream Hello " + req.name());
            }

            stream->Write(reply);
        }
        return Status::OK;
    }
};

TEST_CASE("gRPC one service test", "[GRPC]") {
    const uint16_t port = 50051;
    const std::string addr = "localhost:50051";

    auto service = GreeterServiceImpl<Greeter1>();

    auto server = jsr::rpc::CreateServer(port, service);
    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());

    // wait for server to start
    channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(3));
    auto client = GreeterClient<Greeter1>(channel);

    REQUIRE(client.sayHello("you") == "[1] Hello you");
    REQUIRE(client.sayHelloAgain("you") == "[1] Hello again you");

    client.runChat({"you", "me", "us"});
    REQUIRE(client.getStreamMessages() ==
            std::vector<std::string>({"[1] Stream Hello you", "[1] Stream Hello me", "[1] Stream Hello us"}));

    std::string name = "luoyebai";
    std::thread([&name, &client] { client.startChat(name); }).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(client.getStreamMessages().back() == "[1] Stream Hello luoyebai");
    name = "luoyebaixxx";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(client.getStreamMessages().back() == "[1] Stream Hello luoyebaixxx");
    client.stopChat();

    server->Shutdown();
}

TEST_CASE("gRPC multi service test", "[GRPC]") {
    const uint16_t port = 50052;
    const std::string addr = "localhost:50052";

    auto service1 = GreeterServiceImpl<Greeter1>();
    auto service2 = GreeterServiceImpl<Greeter2>();

    auto server = jsr::rpc::CreateServers(port, service1, service2);

    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(3));

    auto client1 = GreeterClient<Greeter1>(channel);
    auto client2 = GreeterClient<Greeter2>(channel);

    REQUIRE(client1.sayHello("you") == "[1] Hello you");
    REQUIRE(client2.sayHello("you") == "[2] Hello you");

    client1.runChat({"one", "two", "three"});
    client2.runChat({"four", "five", "six"});
    REQUIRE(client1.getStreamMessages() ==
            std::vector<std::string>({"[1] Stream Hello one", "[1] Stream Hello two", "[1] Stream Hello three"}));
    REQUIRE(client2.getStreamMessages() ==
            std::vector<std::string>({"[2] Stream Hello four", "[2] Stream Hello five", "[2] Stream Hello six"}));

    std::string name1 = "joyson";
    std::string name2 = "robot";
    std::thread([&name1, &client1] { client1.startChat(name1); }).detach();
    std::thread([&name2, &client2] { client2.startChat(name2); }).detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(client1.getStreamMessages().back() == "[1] Stream Hello joyson");
    REQUIRE(client2.getStreamMessages().back() == "[2] Stream Hello robot");
    name1 = "Joyson";
    name2 = "Robot";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(client1.getStreamMessages().back() == "[1] Stream Hello Joyson");
    REQUIRE(client2.getStreamMessages().back() == "[2] Stream Hello Robot");

    client1.stopChat();
    client2.stopChat();

    server->Shutdown();
}
}  // namespace jsr_test_grpc