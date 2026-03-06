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

namespace jsr_test_dds {

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;
using namespace jsr::msg;

constexpr size_t MAX_API_ID = 5000;
constexpr auto LR_LOCO_TOPIC = "lr/loco_ctrl";
constexpr auto LOCO_RPC_NAME = "loco";

TEST_CASE("DDS pub/sub benchmark", "[DDS]") {
    jrc::ChannelFactory::instance()->init(0);
    ImuState pubmsg;
    jrc::ChannelPublisher<ImuState> pub(LR_LOCO_TOPIC);
    jrc::ChannelSubscriber<ImuState> sub(
        LR_LOCO_TOPIC, [&pubmsg](const void* msg_ptr) { REQUIRE(*static_cast<const ImuState*>(msg_ptr) == pubmsg); });

    pub.initChannel();
    sub.initChannel();
    BENCHMARK("DDS publish") {
        pub.write(&pubmsg);
    };
}

class LocoServer : public jrr::RpcServer {
   public:
    LocoServer() = default;
    ~LocoServer() = default;
    jrr::Response handleRequest(jrr::Request& request) override {
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

TEST_CASE("Rpc Client/Server communication benchmark case", "[DDS][RPC]") {
    auto api_id = static_cast<int64_t>(random() % MAX_API_ID);
    auto static server = std::make_shared<LocoServer>();
    auto static client = std::make_shared<jrr::RpcClient>();
    server->init(LOCO_RPC_NAME);
    client->init(LOCO_RPC_NAME);
    auto req = jrr::Request(jrr::RequestHeader(api_id), std::string(8000, 'X'));

    BENCHMARK("Rpc Client/Server communication benchmark") {
        auto resp = client->sendApiRequest(req);
        REQUIRE(IsRequestOk(req, resp));
    };
}

TEST_CASE("RPC async benchmark", "[DDS][RPC][ASYNC]") {
    static auto server = std::make_shared<LocoServer>();
    static auto client = std::make_shared<jrr::RpcClient>();

    server->init(LOCO_RPC_NAME);
    client->init(LOCO_RPC_NAME);

    auto api_id = random() % MAX_API_ID;
    jrr::Request req(jrr::RequestHeader(api_id), std::string(8000, 'X'));

    BENCHMARK("RPC async call") {
        client->sendApiRequestAsync(req, [&](const jrr::Response& resp) { REQUIRE(IsRequestOk(req, resp)); });
    };
}

}  // namespace jsr_test_dds

namespace jsr_test_dds_dynamic {

namespace jrc = jsr::robot::channel;

constexpr size_t CAP_SIZE = 100;

TEST_CASE("Dynamic DDS pub/sub benchmark", "[DDS][DYNAMIC]") {
    using namespace jsr::common::dds;

    // ---- Type definition ----
    auto m1_builder = DdsDynamicFactory::createStructType("Member1");
    DdsDynamicFactory::addPrimitiveMember(m1_builder, "first", TK_INT32);
    DdsDynamicFactory::addPrimitiveMember(m1_builder, "second", TK_INT32);
    auto m1_type = m1_builder->build();

    auto m2_builder = DdsDynamicFactory::createStructType("Member2");
    DdsDynamicFactory::addPrimitiveMember(m2_builder, "first", TK_FLOAT32);
    DdsDynamicFactory::addPrimitiveMember(m2_builder, "second", TK_INT64);
    DdsDynamicFactory::addSequenceMember(m2_builder, "seq", m1_type);
    auto m2_type = m2_builder->build();

    auto complex_builder = DdsDynamicFactory::createStructType("ComplexStruct");
    DdsDynamicFactory::addCustomMember(complex_builder, "member1", m1_type);
    DdsDynamicFactory::addCustomMember(complex_builder, "member2", m2_type);
    auto complex_type = complex_builder->build();

    // ---- pub/sub ----
    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>("ComplexTest", complex_builder);

    auto sub =
        std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>("ComplexTest", complex_builder, [](const void* msg) {
            auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);

            auto m1 = data->loan_value(data->get_member_id_by_name("member1"));
            int32_t first{}, second{};
            m1->get_int32_value(first, m1->get_member_id_by_name("first"));
            m1->get_int32_value(second, m1->get_member_id_by_name("second"));
            REQUIRE(first == 12315);
            REQUIRE(second == 12306);

            data->return_loaned_value(m1);
        });

    pub->initChannel();
    sub->initChannel();

    auto data = DdsDynamicFactory::createData(complex_type);

    BENCHMARK("Dynamic DDS publish msgs") {
        auto m1 = data->loan_value(data->get_member_id_by_name("member1"));
        m1->set_int32_value(m1->get_member_id_by_name("first"), 12315);
        m1->set_int32_value(m1->get_member_id_by_name("second"), 12306);
        data->return_loaned_value(m1);

        auto m2 = data->loan_value(data->get_member_id_by_name("member2"));
        m2->set_float32_value(m2->get_member_id_by_name("first"), 666.f);
        m2->set_int64_value(m2->get_member_id_by_name("second"), 0xFFFFF);

        auto seq = m2->loan_value(m2->get_member_id_by_name("seq"));
        for (size_t j = 0; j < CAP_SIZE; ++j) {
            auto elem = seq->loan_value(j);
            elem->set_int32_value(elem->get_member_id_by_name("first"), 12315 + j * 10);
            elem->set_int32_value(elem->get_member_id_by_name("second"), 12306 + j * 10);
            seq->return_loaned_value(elem);
        }
        m2->return_loaned_value(seq);
        data->return_loaned_value(m2);
        pub->write(&data);
    };

    pub->closeChannel();
    sub->closeChannel();
}

constexpr auto TOPIC_NAME = "rt/low_state";
TEST_CASE("Dynamic DDS pub/sub benchmark", "[DDS][DYNAMIC][IDL]") {
    using namespace jsr::common::dds;

    auto builder =
        DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto type = builder->build();

    auto pub = std::make_unique<jrc::ChannelPublisher<DdsDynamicData>>(TOPIC_NAME, builder);

    auto sub = std::make_unique<jrc::ChannelSubscriber<DdsDynamicData>>(TOPIC_NAME, builder, [](const void* msg) {
        auto data = *static_cast<const DdsDynamicData::_ref_type*>(msg);

        auto bms = data->loan_value(data->get_member_id_by_name("bms_state"));
        float voltage{};
        bms->get_float32_value(voltage, bms->get_member_id_by_name("voltage"));
        REQUIRE(voltage == Catch::Approx(48.0f));

        data->return_loaned_value(bms);
    });

    pub->initChannel();
    sub->initChannel();

    auto data = DdsDynamicFactory::createData(type);

    auto bms = data->loan_value(data->get_member_id_by_name("bms_state"));
    auto imu = data->loan_value(data->get_member_id_by_name("imu_state"));
    auto motor_p = data->loan_value(data->get_member_id_by_name("motor_state_parallel"));
    auto motor_s = data->loan_value(data->get_member_id_by_name("motor_state_serial"));

    // BMS
    bms->set_float32_value(bms->get_member_id_by_name("voltage"), 48.0f);
    bms->set_float32_value(bms->get_member_id_by_name("current"), 1.0f);
    bms->set_uint32_value(bms->get_member_id_by_name("cycle"), 10);

    // IMU
    imu->set_float32_values(imu->get_member_id_by_name("rpy"), {0.1f, 0.2f, 0.3f});
    imu->set_float32_values(imu->get_member_id_by_name("gyro"), {0.1f, 0.2f, 0.3f});
    imu->set_float32_values(imu->get_member_id_by_name("acc"), {0.1f, 0.2f, 9.8f});

    // Motors
    for (size_t i = 0; i < CAP_SIZE; ++i) {
        auto p = motor_p->loan_value(i);
        auto s = motor_s->loan_value(i);

        p->set_uint8_value(p->get_member_id_by_name("mode"), 1);
        p->set_float32_value(p->get_member_id_by_name("q"), 0.123f);

        s->set_uint8_value(s->get_member_id_by_name("mode"), 2);
        s->set_float32_value(s->get_member_id_by_name("q"), 1.123f);

        motor_p->return_loaned_value(p);
        motor_s->return_loaned_value(s);
    }

    BENCHMARK("Dynamic DDS publish") {
        pub->write(&data);
    };

    data->return_loaned_value(bms);
    data->return_loaned_value(imu);
    data->return_loaned_value(motor_p);
    data->return_loaned_value(motor_s);

    pub->closeChannel();
    sub->closeChannel();
}

TEST_CASE("Idl Dynamic type serialize benchmark", "[DDS][DYNAMIC][JSON]") {
    using namespace jsr::common::dds;
    auto lowstate_type_builder =
        DdsDynamicFactory::parseTypeFromIdlWithRos2("../../idl/LowState.idl", "jsr::msg::LowState", "../../idl/");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto lowstate_type = lowstate_type_builder->build();
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
    BENCHMARK("DynamicData JSON parse") {
        return DdsDynamicFactory::parseFromJson(data_json, lowstate_type);
    };
    auto data = DdsDynamicFactory::parseFromJson(data_json, lowstate_type);
    BENCHMARK("DynamicData to JSON") {
        return DdsDynamicFactory::toJson(data);
    };

    BENCHMARK("IDL serialize") {
        return DdsDynamicFactory::idlSerialize(lowstate_type);
    };
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
using grpc::Status;

template <class Greeter>
class GreeterClient {
   public:
    using ServiceObject = Greeter;
    using Wrapper = jsr::grpc::ClientWrapper<ServiceObject, HelloRequest, HelloReply>;
    explicit GreeterClient(std::shared_ptr<Channel> channel) : wrapper_(std::make_unique<Wrapper>(channel)) {}

    std::string sayHello(const std::string& user) {
        HelloRequest request{};
        request.set_name(user);
        return wrapper_->rpcCall(request, &ServiceObject::Stub::SayHello).message();
    }

    std::string sayHelloAgain(const std::string& user) {
        HelloRequest request{};
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

TEST_CASE("gRPC single service benchmark", "[GRPC]") {
    const uint16_t port = 50051;
    const std::string addr = "localhost:50051";

    auto service = GreeterServiceImpl<Greeter1>();

    auto server = jsr::grpc::CreateServer(port, service);

    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(3));

    auto client = GreeterClient<Greeter1>(channel);

    BENCHMARK("gRPC SayHello") {
        auto resp = client.sayHello("benchmark");
        REQUIRE(resp == "[1] Hello benchmark");
    };

    BENCHMARK("gRPC SayHelloAgain") {
        auto resp = client.sayHelloAgain("benchmark");
        REQUIRE(resp == "[1] Hello again benchmark");
    };

    server->Shutdown();
}

TEST_CASE("gRPC multi service benchmark", "[GRPC][BENCH]") {
    const uint16_t port = 50052;
    const std::string addr = "localhost:50052";

    auto service1 = GreeterServiceImpl<Greeter1>();
    auto service2 = GreeterServiceImpl<Greeter2>();

    auto server = jsr::grpc::CreateServers(port, service1, service2);

    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(3));

    auto client1 = GreeterClient<Greeter1>(channel);
    auto client2 = GreeterClient<Greeter2>(channel);

    BENCHMARK("gRPC Service1 SayHello") {
        auto resp = client1.sayHello("benchmark");
        REQUIRE(resp == "[1] Hello benchmark");
    };

    BENCHMARK("gRPC Service2 SayHello") {
        auto resp = client2.sayHello("benchmark");
        REQUIRE(resp == "[2] Hello benchmark");
    };

    server->Shutdown();
}

}  // namespace jsr_test_grpc