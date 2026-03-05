#pragma once

// STD
#include <string>

namespace jsr::robot::common {

/**
 * @brief Robot mode
 *
 * @dot
 * digraph {
 *   Damping -> Prepare; Damping -> Custom;
 *   Prepare -> Damping; Prepare -> Walking;
 *   Walking -> Damping; Walking -> Prepare;
 *   Custom  -> Damping; Custom  -> Prepare;
 * }
 * @enddot
 */
enum class RobotMode {
    Unknown = -1,  // For error handling
    Damping = 0,   // All motor enter damping mode, robot will fall down if it is not supported
    Prepare = 1,   // Prepare mode, the robot keeps standing on both feet and can switch to walking mode
    Walking = 2,   // Walking mode, in walking mode, the robot can move, rotate, kick the ball, etc.
    Custom = 3,    // Custom mode, in custom mode, the robot can do some custom actions
};

/**
 * @brief Robot mode to string
 *
 * @param mode Robot mode
 * @return std::string
 */
inline std::string RobotModeToString(RobotMode mode) {
    switch (mode) {
        case RobotMode::Damping:
            return "Damping";
        case RobotMode::Prepare:
            return "Prepare";
        case RobotMode::Walking:
            return "Walking";
        case RobotMode::Custom:
            return "Custom";
        default:
            return "Unknown";
    }
}

/**
 * @brief String to robot mode
 *
 * @param mode Robot mode string
 * @return RobotMode
 */
inline RobotMode StringToRobotMode(const std::string& mode) {
    RobotMode robot_mode = RobotMode::Unknown;
    if (mode == "Damping") {
        robot_mode = RobotMode::Damping;
    } else if (mode == "Prepare") {
        robot_mode = RobotMode::Prepare;
    } else if (mode == "Walking") {
        robot_mode = RobotMode::Walking;
    } else if (mode == "Custom") {
        robot_mode = RobotMode::Custom;
    }

    return robot_mode;
}

enum class Frame {
    Unknown = -1,  // For error handling
    Body = 0,
    Head = 1,
    LeftHand = 2,
    RightHand = 3,
    LeftFoot = 4,
    RightFoot = 5,
};

}  // namespace jsr::robot::common
