// PYBIND11
#include <pybind11/chrono.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
// ROBOT CHANNEL
#include "robot/channel/channel_factory.hpp"
// ROBOT COMMON
#include "robot/common/entities.hpp"
#include "robot/common/robot_shared.hpp"
// ROBOT LITTLE_ROBOT
#include "robot/little_robot/lr_api_const.hpp"
#include "robot/little_robot/lr_loco_api.hpp"
#include "robot/little_robot/lr_loco_client.hpp"
// IDL LR
#include "idl/ButtonEvent.hpp"
#include "idl/ImuState.hpp"
#include "idl/LowCmd.hpp"
#include "idl/LowState.hpp"
#include "idl/MotorCmd.hpp"
#include "idl/MotorState.hpp"
#include "idl/Test.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;
namespace jr = jsr::robot;

using namespace jsr::msg;
namespace jsr::robot::little_robot {

class __attribute__((visibility("hidden"))) LRLowStateSubscriber
    : public std::enable_shared_from_this<LRLowStateSubscriber> {
   public:
    explicit LRLowStateSubscriber(const py::function& py_handler) : py_handler_(py_handler) {}

    void InitChannel() {
        py::gil_scoped_release release;
        auto weak_this = std::weak_ptr<LRLowStateSubscriber>(shared_from_this());
        channel_ptr_ = jr::channel::ChannelFactory::Instance()->CreateRecvChannel<LowState>(
            channel_name_, [weak_this](const void* msg) {
                if (auto shared_this = weak_this.lock()) {
                    {
                        py::gil_scoped_acquire acquire;
                        const LowState* low_state_msg = static_cast<const LowState*>(msg);
                        shared_this->py_handler_(low_state_msg);
                    }
                }
            });
    }

    void CloseChannel() {
        py::gil_scoped_release release;
        if (channel_ptr_) {
            jr::channel::ChannelFactory::Instance()->CloseReader(channel_name_);
            channel_ptr_.reset();
        }
    }

    const std::string& GetChannelName() const { return channel_name_; }

   private:
    jr::channel::ChannelPtr<LowState> channel_ptr_;
    py::function py_handler_;
    const std::string channel_name_ = TopicLowState;
};

class __attribute__((visibility("hidden"))) LRLowCmdPublisher {
   public:
    explicit LRLowCmdPublisher() : channel_name_(TopicJointCtrl) {}

    void InitChannel() {
        py::gil_scoped_release release;
        channel_ptr_ = jr::channel::ChannelFactory::Instance()->CreateSendChannel<LowCmd>(channel_name_);
    }

    bool Write(LowCmd* msg) {
        if (channel_ptr_) {
            return channel_ptr_->Write(msg);
        }
        return false;
    }

    void CloseChannel() {
        py::gil_scoped_release release;
        if (channel_ptr_) {
            jr::channel::ChannelFactory::Instance()->CloseWriter(channel_name_);
            channel_ptr_.reset();
        }
    }

    const std::string& GetChannelName() const { return channel_name_; }

   private:
    std::string channel_name_;
    jr::channel::ChannelPtr<LowCmd> channel_ptr_;
};

}  // namespace jsr::robot::little_robot

PYBIND11_MODULE(jsrsdk_python, m) {
    m.doc() = R"pbdoc(
    python binding of joyson robot sdk
    -----------------------
    )pbdoc";

    py::class_<jr::channel::ChannelFactory>(m, "ChannelFactory")
        .def_static("Instance", &jr::channel::ChannelFactory::Instance, py::return_value_policy::reference,
                    R"pbdoc(
                        Get the singleton instance of the channel factory.

                        Note: The returned instance is managed internally and should not be deleted or modified.
                    )pbdoc")

        .def("Init", py::overload_cast<int32_t, const std::string&>(&jr::channel::ChannelFactory::Init),
             py::arg("domain_id"), py::arg("network_interface") = "",
             R"pbdoc(
                domain_id: domain id of DDS
                network_interface: network interface of DDS, default empty string
            )pbdoc");

    py::enum_<jr::common::RobotMode>(m, "RobotMode")
        .value("Unknown", jr::common::RobotMode::Unknown)
        .value("Damping", jr::common::RobotMode::Damping)
        .value("Prepare", jr::common::RobotMode::Prepare)
        .value("Walking", jr::common::RobotMode::Walking)
        .value("Custom", jr::common::RobotMode::Custom)
        .export_values();

    py::enum_<jr::little_robot::JointIndex>(m, "LRJointIndex")
        .value("LeftShoulderPitch", jr::little_robot::JointIndex::LeftShoulderPitch)
        .value("LeftShoulderRoll", jr::little_robot::JointIndex::LeftShoulderRoll)
        .value("LeftElbowPitch", jr::little_robot::JointIndex::LeftElbowPitch)
        .value("LeftElbowYaw", jr::little_robot::JointIndex::LeftElbowYaw)
        .value("RightShoulderPitch", jr::little_robot::JointIndex::RightShoulderPitch)
        .value("RightShoulderRoll", jr::little_robot::JointIndex::RightShoulderRoll)
        .value("RightElbowPitch", jr::little_robot::JointIndex::RightElbowPitch)
        .value("RightElbowYaw", jr::little_robot::JointIndex::RightElbowYaw)
        .value("WaistYaw", jr::little_robot::JointIndex::WaistYaw)
        .value("WaistRoll", jr::little_robot::JointIndex::WaistRoll)
        .value("LeftHipPitch", jr::little_robot::JointIndex::LeftHipPitch)
        .value("LeftHipRoll", jr::little_robot::JointIndex::LeftHipRoll)
        .value("LeftHipYaw", jr::little_robot::JointIndex::LeftHipYaw)
        .value("LeftKneePitch", jr::little_robot::JointIndex::LeftKneePitch)
        .value("CrankUpLeft", jr::little_robot::JointIndex::CrankUpLeft)
        .value("CrankDownLeft", jr::little_robot::JointIndex::CrankDownLeft)
        .value("RightHipPitch", jr::little_robot::JointIndex::RightHipPitch)
        .value("RightHipRoll", jr::little_robot::JointIndex::RightHipRoll)
        .value("RightHipYaw", jr::little_robot::JointIndex::RightHipYaw)
        .value("RightKneePitch", jr::little_robot::JointIndex::RightKneePitch)
        .value("CrankUpRight", jr::little_robot::JointIndex::CrankUpRight)
        .value("CrankDownRight", jr::little_robot::JointIndex::CrankDownRight)
        .export_values();
    m.attr("LRJointCnt") = jr::little_robot::JointCnt;

    py::enum_<jr::little_robot::LocoApiId>(m, "LRLocoApiId")
        .value("ChangeMode", jr::little_robot::LocoApiId::ChangeMode)
        .value("Move", jr::little_robot::LocoApiId::Move)
        .value("RotateHead", jr::little_robot::LocoApiId::RotateHead)
        .export_values();

    // clang-format off
#include "idl/Test.pybind.ipp"
#include "idl/ButtonEvent.pybind.ipp"
#include "idl/BmsState.pybind.ipp"
#include "idl/ImuState.pybind.ipp"
#include "idl/MotorState.pybind.ipp"
#include "idl/LowState.pybind.ipp"
#include "idl/MotorCmd.pybind.ipp"
#include "idl/LowCmd.pybind.ipp"
    // clang-format on

    // ROBOT
    py::class_<jr::little_robot::LRLowStateSubscriber, std::shared_ptr<jr::little_robot::LRLowStateSubscriber>>(
        m, "LRLowStateSubscriber")
        .def(py::init<const py::function&>(), py::arg("handler"), R"pbdoc(
                 /**
                 * @brief init low state subscriber with callback handler
                 *
                 * @param handler callback handler of low state, the handler should accept one parameter of type LowState
                 *
                 */
            )pbdoc")
        .def("InitChannel", &jr::little_robot::LRLowStateSubscriber::InitChannel, "Init low state subscription channel")
        .def("CloseChannel", &jr::little_robot::LRLowStateSubscriber::CloseChannel,
             "Close low state subscription channel")
        .def("GetChannelName", &jr::little_robot::LRLowStateSubscriber::GetChannelName,
             "Get low state subscription channel name");

    py::class_<jr::little_robot::LRLowCmdPublisher>(m, "LRLowCmdPublisher")
        .def(py::init<>())
        .def("InitChannel", &jr::little_robot::LRLowCmdPublisher::InitChannel, "Init low cmd publication channel")
        .def("Write", &jr::little_robot::LRLowCmdPublisher::Write, py::arg("msg"), R"pbdoc(
                 /**
                 * @brief write low cmd message into channel, i.e. publish low cmd message
                 *
                 * @param msg LowCmd
                 *
                 */
            )pbdoc")
        .def("CloseChannel", &jr::little_robot::LRLowCmdPublisher::CloseChannel, "Close low cmd publication channel")
        .def("GetChannelName", &jr::little_robot::LRLowCmdPublisher::GetChannelName,
             "Get low cmd publication channel name");

    py::class_<jr::little_robot::LRLocoClient, std::shared_ptr<jr::little_robot::LRLocoClient>>(m, "LRLocoClient",
                                                                                                R"pbdoc(
                LRLocoClient is a client interface for controlling the little robot's locomotion and other high-level functionalities.
                It provides methods to send API requests, change robot modes, move the robot, control its head and hands, and more.
        )pbdoc")
        .def(py::init<>())
        .def(
            "Init",
            [](jr::little_robot::LRLocoClient& client) {
                py::gil_scoped_release release;
                return client.Init();
            },
            "Init")
        .def(
            "Init",
            [](jr::little_robot::LRLocoClient& client, const std::string& robot_name) {
                py::gil_scoped_release release;
                return client.Init(robot_name);
            },
            "Init with robot name")
        .def("SendApiRequest", &jr::little_robot::LRLocoClient::SendApiRequest, py::arg("api_id"), py::arg("param"),
             R"pbdoc(
                /**
                 * @brief Send API request to little robot
                 *
                 * @param api_id API_ID, you can find the API_ID in lr_loco_api.hpp
                 * @param param API parameter
                 *
                 * @return 0 if success, otherwise return error code
                 */
            )pbdoc")
        .def("ChangeMode", &jr::little_robot::LRLocoClient::ChangeMode, py::arg("mode"),
             R"pbdoc(
                /**
                 * @brief Change robot mode
                 *
                 * @param mode robot mode, options are: Damping, Prepare, Walking, Custom
                 *
                 * @return 0 if success, otherwise return error code
                 */
            )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
