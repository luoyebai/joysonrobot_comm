
#pragma once

// STD
#include <memory>
// ROBOT COMMON
#include "robot/common/robot_shared.hpp"
// ROBOT LR
#include "robot/little_robot/lr_loco_api.hpp"
// ROBOT RPC
#include "robot/rpc/rpc_client.hpp"

namespace jsr::robot::little_robot {

/**
 * @brief Little Robot locomotion client
 *
 */
class LRLocoClient {
   public:
    LRLocoClient() = default;
    ~LRLocoClient() = default;

    void Init() {
        rpc_client_ = std::make_shared<rpc::RpcClient>();
        rpc_client_->Init(LOCO_SERVICE_NAME);
        return;
    }

    void Init(const std::string& robot_name) {
        rpc_client_ = std::make_shared<rpc::RpcClient>();
        rpc_client_->Init(robot_name + LOCO_SERVICE_NAME);
        return;
    }
    /**
     * @brief Send API request to B1 robot
     *
     * @param api_id API_ID, you can find the API_ID in b1_api_const.hpp
     * @param param API parameter
     *
     * @return 0 if success, otherwise return error code
     */
    int32_t SendApiRequest(const LocoApiId api_id, const std::string& param) {
        auto req = rpc::Request();
        auto header = rpc::RequestHeader();
        header.SetApiId(static_cast<int64_t>(api_id));
        req.SetHeader(header);
        req.SetBody(param);
        auto resp = rpc_client_->SendApiRequest(req);
        return resp.GetHeader().GetStatus();
    }

    /**
     * @brief Send API request to B1 robot with response
     *
     * @param api_id API_ID, you can find the API_ID in b1_api_const.hpp
     * @param param API parameter
     * @param resp [out] A reference to a Response object where the API's response will be stored.
     * This parameter is modified by the function to contain the result of the API call
     *
     * @return 0 if success, otherwise return error code
     */
    int32_t SendApiRequestWithResponse(const LocoApiId api_id, const std::string& param, rpc::Response& resp) {
        auto req = rpc::Request();
        auto header = rpc::RequestHeader();
        header.SetApiId(static_cast<int64_t>(api_id));
        req.SetHeader(header);
        req.SetBody(param);
        resp = rpc_client_->SendApiRequest(req);
        return resp.GetHeader().GetStatus();
    }

    /**
     * @brief Change robot mode
     *
     * @param mode robot mode, options are: kDamping, kPrepare, kWalking
     *
     * @return 0 if success, otherwise return error code
     */
    int32_t ChangeMode(jsr::robot::common::RobotMode mode) {
        auto change_mode = ChangeModeParameter(mode);
        std::string param = change_mode.ToJson().dump();
        return SendApiRequest(LocoApiId::ChangeMode, param);
    }

   private:
    std::shared_ptr<rpc::RpcClient> rpc_client_;
};

}  // namespace jsr::robot::little_robot
