// STD
#include <memory>
// RPC
#include "robot/rpc/error.hpp"
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC SERVER
#include "robot/rpc/rpc_client.hpp"
#include "robot/rpc/rpc_server.hpp"

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;

constexpr auto LOCO_SERVER_NAME = "loco";

enum class LocoApiId {
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

class LocoServer : public jrr::RpcServer {
   public:
    LocoServer() = default;
    ~LocoServer() = default;

   private:
    jrr::Response HandleRequest(jrr::Request& request) override {
        auto api_id = request.GetHeader().GetApiId();
        auto response = jrr::Response();
        switch (static_cast<LocoApiId>(api_id)) {
            case LocoApiId::ChangeMode:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get change mode response");
                break;
            case LocoApiId::Move:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get move response");
                break;
            case LocoApiId::Run:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeServerRefused));
                response.SetBody("Get run response");
                break;
            case LocoApiId::RotateHead:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get rotate head response");
                break;
            case LocoApiId::WaveHand:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get wave hand response");
                break;
            case LocoApiId::RotateHeadWithDirection:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get rotate head with direction response");
                break;
            case LocoApiId::LieDown:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get lie down response");
                break;
            case LocoApiId::GetUp:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get get up response");
                break;
            case LocoApiId::MoveHandEndEffector:
                break;
            case LocoApiId::ControlGripper:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get control gripper response");
                break;
            case LocoApiId::GetFrameTransform:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get frame transform response");
                break;
            case LocoApiId::SwitchHandEndEffectorControlMode:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get switch hand end effector control mode response");
                break;
            case LocoApiId::ControlDexterousHand:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get control dexterous hand response");
                break;
            case LocoApiId::Handshake:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeSuccess));
                response.SetBody("Get handshake response");
                break;
            default:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeInvalid));
                response.SetBody("Get unknown request");
        }
        // response.SetBody(request.GetBody());
        fmt::print("[Server]| Get Request with api id={},body={}\n", api_id, response.GetBody());
        return response;
    }
};

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto server = std::make_shared<LocoServer>();
    server->Init(LOCO_SERVER_NAME);
    // The server will run for 100 seconds
    for (size_t i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
