#pragma once

// STD
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

// ROBOT COMMON
#include "robot/common/robot_shared.hpp"

namespace jsr::robot::common {

using ChangeStatePath = std::unordered_map<int, std::string>;

// Common FSM state
static const auto DAMPING_STATE =
    std::pair<int, std::string>(static_cast<int>(RobotMode::Damping), RobotModeToString(RobotMode::Damping));
static const auto PREPARE_STATE =
    std::pair<int, std::string>(static_cast<int>(RobotMode::Prepare), RobotModeToString(RobotMode::Prepare));
static const auto WALKING_STATE =
    std::pair<int, std::string>(static_cast<int>(RobotMode::Walking), RobotModeToString(RobotMode::Walking));
static const auto CUSTOM_STATE =
    std::pair<int, std::string>(static_cast<int>(RobotMode::Custom), RobotModeToString(RobotMode::Custom));
static const ChangeStatePath DAMPING_PATH = {DAMPING_STATE, PREPARE_STATE, CUSTOM_STATE};
static const ChangeStatePath PREPARE_PATH = {DAMPING_STATE, PREPARE_STATE, WALKING_STATE};
static const ChangeStatePath WALKING_PATH = {DAMPING_STATE, PREPARE_STATE, WALKING_STATE};
static const ChangeStatePath CUSTOM_PATH = {DAMPING_STATE, PREPARE_STATE, CUSTOM_STATE};

/**
 * @brief FSMState class,Implement the state machine by registering new state nodes
 * and state transfer paths as well as some state callback functions
 *
 */
class FSMState {
   public:
    using StateFunc = std::function<void()>;
    struct StateStruct {
        ChangeStatePath path;
        StateFunc run_func;
        StateFunc enter_func;
        StateFunc exit_func;
    };

    using ModeList = std::map<std::string, StateStruct>;

    FSMState() = default;
    ~FSMState() = default;

    bool ChangeMode(int id) {
        if (state_list_.count(current_state_) == 0) {
            throw std::runtime_error("Current state not registered: " + current_state_);
        }
        const auto [path, run, _, exit] = state_list_[current_state_];
        // Get next state
        if (auto it = path.find(id); it != path.end()) {
            if (current_state_ == it->second && run_func_ != nullptr) {
                return true;
            }
            // enter next state
            current_state_ = it->second;
            if (state_list_.count(current_state_) == 0) {
                throw std::runtime_error("Current state not registered: " + current_state_);
                return false;
            }

            // exit current state
            if (exit) {
                exit();
            }

            const auto current_state = state_list_[current_state_];
            if (current_state.enter_func) {
                current_state.enter_func();
            }
            run_func_ = current_state.run_func;
            return true;
        }
        return false;
    }

    std::string GetNowStateName() const { return current_state_; }

    FSMState& RegisterState(const std::string& state_name, ChangeStatePath path, StateFunc run,
                            StateFunc enter = nullptr, StateFunc exit = nullptr) {
        if (current_state_.empty()) {
            current_state_ = state_name;
        }
        state_list_.insert(
            {state_name, StateStruct{std::move(path), std::move(run), std::move(enter), std::move(exit)}});
        return *this;
    };

    void Run() {
        if (!run_func_) {
            throw std::runtime_error("Current state function is nullptr");
        }
        run_func_();
        return;
    };

   private:
    std::string current_state_;
    StateFunc run_func_ = nullptr;
    ModeList state_list_;
};

using FSMStatePtr = std::shared_ptr<FSMState>;

}  // namespace jsr::robot::common
