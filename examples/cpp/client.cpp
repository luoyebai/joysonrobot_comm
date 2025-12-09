// STD
#include <memory>
// RPC
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC CLIENT
#include "robot/rpc/rpc_client.hpp"

constexpr auto LOCO_SERVER_NAME = "loco";

namespace jrc = jsr::robot::channel;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto client = std::make_shared<jsr::robot::rpc::RpcClient>();
    client->Init(LOCO_SERVER_NAME);
    size_t frame_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    int api = 0;
    jsr::robot::rpc::Request req;
    jsr::robot::rpc::RequestHeader header;
    while (true) {
        fmt::print("\nEnter API number: ");
        std::cin >> api;
        header.SetApiId(api);
        req.SetHeader(header);
        req.SetBody("This is a message");
        client->SendApiRequest(req);
        frame_count++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();

        if (elapsed >= 1) {
            fmt::print("FPS: {}\n", frame_count);
            frame_count = 0;
            last_time = now;
        }
    }
}