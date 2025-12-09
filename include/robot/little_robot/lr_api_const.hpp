#pragma once

#include <array>
#include <string>

namespace jsr::robot::little_robot {

static const std::string TopicJointCtrl = "rt/joint_ctrl";
static const std::string TopicLowState = "rt/low_state";
static const std::string TopicOdometerState = "rt/odometer_state";

// TODO(@Tanjiachun): Update joint index
enum class JointIndex {
    // Left arm
    LeftShoulderPitch = 0,
    LeftShoulderRoll = 1,
    LeftElbowYaw = 2,
    LeftElbowPitch = 3,

    // Right arm
    RightShoulderPitch = 4,
    RightShoulderRoll = 5,
    RightElbowYaw = 6,
    RightElbowPitch = 7,

    // waist
    WaistYaw = 8,
    WaistRoll = 9,

    // left leg
    LeftHipPitch = 10,
    LeftHipRoll = 11,
    LeftHipYaw = 12,
    LeftKneePitch = 13,
    CrankUpLeft = 14,
    CrankDownLeft = 15,

    // right leg
    RightHipPitch = 16,
    RightHipRoll = 17,
    RightHipYaw = 18,
    RightKneePitch = 19,
    CrankUpRight = 20,
    CrankDownRight = 21,
};

enum class JointIndexWith7DofArm {
    // head
    HeadYaw = 0,
    HeadPitch = 1,

    // Left arm
    LeftShoulderPitch = 2,
    LeftShoulderRoll = 3,
    LeftElbowPitch = 4,
    LeftElbowYaw = 5,
    LeftWristPitch = 6,
    LeftWristYaw = 7,
    LeftHandRoll = 8,

    // Right arm
    RightShoulderPitch = 9,
    RightShoulderRoll = 10,
    RightElbowPitch = 11,
    RightElbowYaw = 12,
    RightWristPitch = 13,
    RightWristYaw = 14,
    RightHandRoll = 15,

    // waist
    Waist = 16,

    // left leg
    LeftHipPitch = 17,
    LeftHipRoll = 18,
    LeftHipYaw = 19,
    LeftKneePitch = 20,
    CrankUpLeft = 21,
    CrankDownLeft = 22,

    // right leg
    RightHipPitch = 23,
    RightHipRoll = 24,
    RightHipYaw = 25,
    RightKneePitch = 26,
    CrankUpRight = 27,
    CrankDownRight = 28
};

static const size_t JointCnt = 22;
static const size_t JointCnt7DofArm = 29;

using KpsType = std::array<float, JointCnt>;
using KdsType = std::array<float, JointCnt>;

// using KpsType = std::array<float, JointCnt7DofArm>;
// using KdsType = std::array<float, JointCnt7DofArm>;

enum HandIndex {
    LeftHand = 0,
    RightHand = 1,
};

enum HandAction {
    HandOpen = 0,
    HandClose = 1,
};

}  // namespace jsr::robot::little_robot
