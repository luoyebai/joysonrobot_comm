// STD
#include <memory>
// RPC
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC CLIENT
#include "robot/little_robot/lr_loco_client.hpp"

namespace jrc = jsr::robot::channel;
using namespace jsr::robot::little_robot;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto client = std::make_shared<LRLocoClient>();
    client->Init();
    size_t frame_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    int mode = 0;
    while (true) {
        fmt::print("Enter mode(0: Damping, 1: Prepare, 2: Walking, 3: Custom):");
        std::cin >> mode;
        switch (mode) {
            case 0:
                client->ChangeMode(jsr::robot::common::RobotMode::Damping);
                break;
            case 1:
                client->ChangeMode(jsr::robot::common::RobotMode::Prepare);
                break;
            case 2:
                client->ChangeMode(jsr::robot::common::RobotMode::Walking);
                break;
            case 3:
                client->ChangeMode(jsr::robot::common::RobotMode::Custom);
                break;
            default:
                client->ChangeMode(jsr::robot::common::RobotMode::Unknown);
                break;
        }

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