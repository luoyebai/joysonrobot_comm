// STD
#include <memory>
// RPC
#include "jsrcomm/robot/rpc/error.hpp"
#include "jsrcomm/robot/rpc/request.hpp"
#include "jsrcomm/robot/rpc/response.hpp"
// RPC SERVER
#include "jsrcomm/robot/rpc/rpc_client.hpp"
#include "jsrcomm/robot/rpc/rpc_server.hpp"

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;

constexpr auto SERVER_NAME = "loco";

enum class ApiId {
    Exit = 1999,
    ChangeMode = 2000,
    Move = 2001,
    Run = 2002,
    RotateHead = 2004,
    WaveHand = 2005,
    RotateHeadWithDirection = 2006,
    LieDown = 2007,
    GetUp = 2008,
    MoveHandEndEffector = 2009,
    ControlGripper = 2010,
    GetFrameTransform = 2011,
    SwitchHandEndEffectorControlMode = 2012,
    ControlDexterousHand = 2013,
    Handshake = 2015
};

class Server : public jrr::RpcServer {
   public:
    Server() = default;
    ~Server() = default;

    std::atomic_flag stopped = ATOMIC_FLAG_INIT;

   private:
    jrr::Response handleRequest(jrr::Request& request) override {
        auto api_id = request.GetHeader().GetApiId();
        auto response = jrr::Response();
        switch (static_cast<ApiId>(api_id)) {
            case ApiId::Exit:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Exit server");
                break;
            case ApiId::ChangeMode:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get change mode response");
                break;
            case ApiId::Move:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get move response");
                break;
            case ApiId::Run:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SERVER_REFUSED));
                response.SetBody("Get run response");
                break;
            case ApiId::RotateHead:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get rotate head response");
                break;
            case ApiId::WaveHand:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get wave hand response");
                break;
            case ApiId::RotateHeadWithDirection:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get rotate head with direction response");
                break;
            case ApiId::LieDown:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get lie down response");
                break;
            case ApiId::GetUp:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get get up response");
                break;
            case ApiId::MoveHandEndEffector:
                break;
            case ApiId::ControlGripper:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get control gripper response");
                break;
            case ApiId::GetFrameTransform:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get frame transform response");
                break;
            case ApiId::SwitchHandEndEffectorControlMode:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get switch hand end effector control mode response");
                break;
            case ApiId::ControlDexterousHand:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get control dexterous hand response");
                break;
            case ApiId::Handshake:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_SUCCESS));
                response.SetBody("Get handshake response");
                break;
            default:
                response.SetHeader(jrr::ResponseHeader(jrr::RPC_STATUS_CODE_INVALID));
                response.SetBody("Get unknown request");
        }
        fmt::print("[Server]| Get Request with api id={},body={}\n", api_id, response.GetBody());
        if (api_id == static_cast<size_t>(ApiId::Exit)) {
            stopped.test_and_set();
        }
        return response;
    }
};

int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto server = std::make_shared<Server>();
    server->init(SERVER_NAME);
    // The server will run for 100 seconds
    constexpr size_t SLEEP_COUNT = 100;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < SLEEP_COUNT; ++i) {
        if (server->stopped._M_i) {
            fmt::print("[Server] | Stoped...\n");
            server->Stop();
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
        fmt::print("[Server] | Running...\n");
    }
    fmt::print("[Server] | Process Over!\n");
}
