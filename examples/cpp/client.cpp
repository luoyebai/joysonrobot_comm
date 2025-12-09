// STD
#include <memory>
// RPC
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC CLIENT
#include "robot/rpc/rpc_client.hpp"

constexpr auto SERVER_NAME = "loco";

namespace jrc = jsr::robot::channel;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto client = std::make_shared<jsr::robot::rpc::RpcClient>();
    client->Init(SERVER_NAME);
    size_t frame_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    jsr::robot::rpc::Request req;
    jsr::robot::rpc::RequestHeader header;

    for (size_t i = 0; i < 5000; ++i) {
        auto api = random() % 16 + 2000;
        header.SetApiId(api);
        req.SetHeader(header);
        req.SetBody("This is a message");

        // sync send and async send
        auto resp = client->SendApiRequest(req);
        fmt::print("Response: {}\n", resp.GetBody());
        client->SendApiRequestAsync(req, [](const jsr::robot::rpc::Response& resp) {
            fmt::print("Async Response: {}\n", resp.GetBody());
            return;
        });
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));

        frame_count++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();

        if (elapsed >= 1) {
            fmt::print("FPS: {}\n", frame_count);
            frame_count = 0;
            last_time = now;
        }
    }
    // Exit
    auto api = 1999;
    header.SetApiId(api);
    req.SetHeader(header);
    req.SetBody("Exit");
    client->SendApiRequestAsync(req, [](const jsr::robot::rpc::Response& resp) {});
}