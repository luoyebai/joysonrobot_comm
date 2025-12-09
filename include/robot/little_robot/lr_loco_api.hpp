#pragma once

// STD
#include <string>
// ROBOT COMMON
#include "robot/common/entities.hpp"
#include "robot/common/robot_shared.hpp"
// ROBOT LR
#include "robot/little_robot/lr_api_const.hpp"
// JSON
#include "serialization/json.hpp"

namespace jsr::robot::little_robot {

/* service name */
const std::string LOCO_SERVICE_NAME = "loco";

/*API version*/
const std::string LOCO_API_VERSION = "1.0.0.1";

/*API ID */
enum class LocoApiId {
    ChangeMode = 2000,
    Move = 2001,
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

class ChangeModeParameter {
   public:
    ChangeModeParameter() = default;
    explicit ChangeModeParameter(jsr::robot::common::RobotMode mode) : mode_(mode) {}

   public:
    void FromJson(nlohmann::json& json) { mode_ = static_cast<jsr::robot::common::RobotMode>(json["mode"]); }

    nlohmann::json ToJson() const {
        auto json = nlohmann::json();
        json["mode"] = static_cast<int>(mode_);
        return json;
    }

   public:
    jsr::robot::common::RobotMode mode_;
};

}  // namespace jsr::robot::little_robot