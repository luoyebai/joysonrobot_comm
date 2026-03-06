//STD
#include <random>
// TEST
#define CATCH_CONFIG_MAIN
#include "test_tools/catch_amalgamated.hpp"
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

constexpr auto LR_LOCO_TOPIC = "lr/loco_ctrl";
constexpr auto LOCO_RPC_NAME = "loco";

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;
using namespace jsr::msg;

TEST_CASE("Publisher/Subscriber DDS communication test cases", "[PUBSUB]") {
    const size_t test_count = 10000;
    auto g_f = [&]() {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return static_cast<float>(dist(rng));
    };

    auto pubmsg = ImuState();
    jrc::ChannelFactory::instance()->init(0);
    jrc::ChannelPublisher<ImuState> pub(LR_LOCO_TOPIC);
    jrc::ChannelSubscriber<ImuState> sub(LR_LOCO_TOPIC, [&pubmsg](const void* msg_ptr) {
        const auto* msg = static_cast<const ImuState*>(msg_ptr);
        REQUIRE(*msg == pubmsg);
    });

    pub.initChannel();
    sub.initChannel();
    for (size_t i = 0; i < test_count; ++i) {
        pubmsg.gyro({g_f(), g_f(), g_f()});
        pubmsg.acc({g_f(), g_f(), g_f()});
        pubmsg.rpy({g_f(), g_f(), g_f()});
        pub.write(&pubmsg);
    }

    BENCHMARK("Publisher DDS communication benchmark") {
        for (size_t i = 0; i < test_count / 100; ++i) {
            pub.write(&pubmsg);
        }
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

bool isRequestOk(const jrr::Request& req, const jrr::Response& resp) {
    // No error handling in server
    return (resp.GetHeader().GetStatus() == jrr::RPC_STATUS_CODE_SUCCESS && req.GetBody() == resp.GetBody());
}

TEST_CASE("Rpc Client/Server communication test cases", "[Client/Server]") {
    const size_t test_count = 10000;
    auto api_id = static_cast<int64_t>(random() % 5000);
    jrc::ChannelFactory::instance()->init(0);
    auto static server = std::make_shared<LocoServer>();
    auto static client = std::make_shared<jrr::RpcClient>();
    server->init(LOCO_RPC_NAME);
    client->init(LOCO_RPC_NAME);
    auto req = jrr::Request(jrr::RequestHeader(api_id), std::string(8000, 'X'));
    for (size_t i = 0; i < test_count; ++i) {
        auto resp = client->sendApiRequest(req);
        REQUIRE(isRequestOk(req, resp));
    }

    BENCHMARK("Rpc Client/Server communication benchmark") {
        for (size_t i = 0; i < test_count / 100; ++i) {
            auto resp = client->sendApiRequest(req);
            REQUIRE(isRequestOk(req, resp));
        }
    };
}

TEST_CASE("Rpc Client/Server communication aysnc test cases", "[Client/Server async]") {
    const size_t test_count = 10000;
    auto api_id = static_cast<int64_t>(random() % 5000);
    jrc::ChannelFactory::instance()->init(0);
    auto static server = std::make_shared<LocoServer>();
    auto static client = std::make_shared<jrr::RpcClient>();
    server->init(LOCO_RPC_NAME);
    client->init(LOCO_RPC_NAME);
    auto req = jrr::Request(jrr::RequestHeader(api_id), std::string(8000, 'X'));

    std::atomic<size_t> done_count{0};
    for (size_t i = 0; i < test_count; ++i) {
        client->sendApiRequestAsync(req, [&](const jrr::Response& resp) {
            REQUIRE(isRequestOk(req, resp));
            done_count.fetch_add(1, std::memory_order_relaxed);
            return;
        });
    }

    const auto start = std::chrono::steady_clock::now();
    while (done_count.load() < test_count) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
            FAIL("Async RPC test timed out. Not all callbacks finished.");
            break;
        }
    }

    BENCHMARK("Rpc Client/Server communication async benchmark") {
        for (size_t i = 0; i < test_count / 100; ++i) {
            client->sendApiRequestAsync(req, [&](const jrr::Response& resp) {
                REQUIRE(isRequestOk(req, resp));
                return;
            });
        }
    };
}

/**
 * @brief Test DDS communication MockServer, Don't use stop function in the handleRequest function
 * You should use atomic_bool to stop the server
 *
 */
class MockServer : public jrr::RpcServer {
   public:
    std::atomic_flag stopped = ATOMIC_FLAG_INIT;

   private:
    jrr::Response handleRequest(jrr::Request& request) override {
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

TEST_CASE("Rpc Client/Server communication test cases with timeout", "[Client/Server timeout]") {
    jrc::ChannelFactory::instance()->init(0);
    auto static server = std::make_unique<MockServer>();
    auto static client = std::make_unique<jrr::RpcClient>();
    server->init("TimeoutTest");
    client->init("TimeoutTest");
    int64_t status = -1;
    auto req = jrr::Request(jrr::RequestHeader(0), "");
    status = client->sendApiRequest(req).GetHeader().GetStatus();
    REQUIRE(status == jrr::RPC_STATUS_CODE_SUCCESS);
    if (server->stopped._M_i) {
        server->stop();
    }
    status = client->sendApiRequest(req).GetHeader().GetStatus();
    REQUIRE(status == jrr::RPC_STATUS_CODE_TIMEOUT);
    client->stop();
}