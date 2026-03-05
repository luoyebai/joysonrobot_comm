// STD
#include <memory>
// RPC
#include "jsrcomm/robot/rpc/request.hpp"
#include "jsrcomm/robot/rpc/response.hpp"
// RPC CLIENT
#include "jsrcomm/robot/rpc/rpc_client.hpp"

constexpr auto SERVER_NAME = "loco";

namespace jrc = jsr::robot::channel;

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto client = std::make_shared<jsr::robot::rpc::RpcClient>();
    client->init(SERVER_NAME);
    size_t frame_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    auto req = jsr::robot::rpc::Request();
    auto header = jsr::robot::rpc::RequestHeader();

    constexpr size_t REQUEST_COUNT = 5000;
    constexpr size_t API_BASE_SIZE = 2000;
    constexpr size_t API_SIZE = 16;
    for (size_t i = 0; i < REQUEST_COUNT; ++i) {
        auto api = random() % API_SIZE + API_BASE_SIZE;
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
    // 1999: Exit
    auto api = API_BASE_SIZE - 1;
    header.SetApiId(api);
    req.SetHeader(header);
    req.SetBody("Exit");
    client->SendApiRequestAsync(req, [](const jsr::robot::rpc::Response& resp) {});
}
