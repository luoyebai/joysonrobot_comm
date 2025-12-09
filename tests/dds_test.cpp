//STD
#include <random>
// TEST
#define CATCH_CONFIG_MAIN
#include "test_tools/catch_amalgamated.hpp"
// ROBOT CHANNEL
#include "robot/channel/channel_publisher.hpp"
#include "robot/channel/channel_subscriber.hpp"
// RPC
#include "robot/rpc/error.hpp"
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC CLIENT/SERVER
#include "robot/rpc/rpc_client.hpp"
#include "robot/rpc/rpc_server.hpp"
//IDL
#include "idl/ImuState.hpp"

constexpr auto LR_LOCO_TOPIC = "lr/loco_ctrl";
constexpr auto LOCO_RPC_NAME = "loco";

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;
using namespace jsr::msg;

TEST_CASE("Publisher/Subscriber DDS communication test cases", "[PUBSUB]") {
    const size_t test_count = 10000;
    auto g_f = [&]() {
        static std::mt19937 rng(123);
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return static_cast<float>(dist(rng));
    };

    auto pubmsg = ImuState();
    jrc::ChannelFactory::Instance()->Init(0);
    jrc::ChannelPublisher<ImuState> pub(LR_LOCO_TOPIC);
    jrc::ChannelSubscriber<ImuState> sub(LR_LOCO_TOPIC, [&pubmsg](const void* msg_ptr) {
        const auto* msg = static_cast<const ImuState*>(msg_ptr);
        REQUIRE(*msg == pubmsg);
    });

    pub.InitChannel();
    sub.InitChannel();
    for (size_t i = 0; i < test_count; ++i) {
        pubmsg.gyro({g_f(), g_f(), g_f()});
        pubmsg.acc({g_f(), g_f(), g_f()});
        pubmsg.rpy({g_f(), g_f(), g_f()});
        pub.Write(&pubmsg);
    }

    BENCHMARK("Publisher DDS communication benchmark") {
        for (size_t i = 0; i < test_count / 10; ++i) {
            pub.Write(&pubmsg);
        }
    };
}

class LocoServer : public jrr::RpcServer {
   public:
    LocoServer() = default;
    ~LocoServer() = default;
    jrr::Response HandleRequest(jrr::Request& request) override {
        auto response = jrr::Response();
        response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
        response.SetBody(request.MoveBody());
        return response;
    }
};

bool isRequestOk(const jrr::Request& req, const jrr::Response& resp) {
    // No error handling in server
    return (resp.GetHeader().GetStatus() == jrr::RpcStatusCodeSuccess && req.GetBody() == resp.GetBody());
}

TEST_CASE("Rpc Client/Server communication test cases", "[Client/Server]") {
    const size_t test_count = 10000;
    auto api_id = static_cast<int64_t>(random() % 5000);
    jrc::ChannelFactory::Instance()->Init(0);
    auto static server = std::make_shared<LocoServer>();
    auto static client = std::make_shared<jrr::RpcClient>();
    server->Init(LOCO_RPC_NAME);
    client->Init(LOCO_RPC_NAME);
    auto req = jrr::Request(jrr::RequestHeader(api_id), std::string(8000, 'X'));
    for (size_t i = 0; i < test_count; ++i) {
        auto resp = client->SendApiRequest(req);
        REQUIRE(isRequestOk(req, resp));
    }

    BENCHMARK("Rpc Client/Server communication benchmark") {
        for (size_t i = 0; i < test_count / 10; ++i) {
            auto resp = client->SendApiRequest(req);
            REQUIRE(isRequestOk(req, resp));
        }
    };
}