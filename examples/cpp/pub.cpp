//STD
#include <chrono>
#include <memory>
#include <string>
#include <thread>
// PUB
#include "jsrcomm/robot/channel/channel_publisher.hpp"
// IDL
#include "jsrcomm/idl/BmsState.hpp"
#include "jsrcomm/idl/ImuState.hpp"
#include "jsrcomm/idl/LowState.hpp"

constexpr auto TOPIC_NUM = 3;
constexpr std::array<const char*, TOPIC_NUM> TOPIC_NAMES = {"rt/low_state", "rt/imu_state", "rt/bms_state"};

namespace jrc = jsr::robot::channel;

using namespace jsr::msg;
int main() {
    jrc::ChannelFactory::instance()->init(0);
    auto pub1 = std::make_unique<jrc::ChannelPublisher<LowState>>(TOPIC_NAMES[0]);
    auto pub2 = std::make_unique<jrc::ChannelPublisher<ImuState>>(TOPIC_NAMES[1]);
    auto pub3 = std::make_unique<jrc::ChannelPublisher<BmsState>>(TOPIC_NAMES[2]);
    pub1->initChannel();
    pub2->initChannel();
    pub3->initChannel();
    auto msg1 = LowState();
    auto msg2 = ImuState();
    auto msg3 = BmsState();

    constexpr auto MOTOR_NUM = 10;
    msg1.motor_state_serial().resize(MOTOR_NUM);
    msg1.motor_state_parallel().resize(MOTOR_NUM);
    for (size_t i = 0; i < MOTOR_NUM; ++i) {
        msg1.motor_state_serial()[i].q(1 + 0.1 * i);
        msg1.motor_state_serial()[i].dq(2 + 0.1 * i);
        msg1.motor_state_serial()[i].ddq(3 + 0.1 * i);
        msg1.motor_state_serial()[i].tau_est(4 + 0.1 * i);
        msg1.motor_state_parallel()[i].q(1 - 0.1 * i);
        msg1.motor_state_parallel()[i].dq(2 - 0.1 * i);
        msg1.motor_state_parallel()[i].ddq(3 - 0.1 * i);
        msg1.motor_state_parallel()[i].tau_est(4 - 0.1 * i);
    }
    msg1.imu_state().acc()[0] = 0.1;
    msg1.imu_state().acc()[1] = 0.2;
    msg1.imu_state().acc()[2] = 0.3;
    msg1.imu_state().rpy()[0] = 0.4;
    msg1.imu_state().rpy()[1] = 0.5;
    msg1.imu_state().rpy()[2] = 0.6;
    msg1.imu_state().gyro()[0] = 0.7;
    msg1.imu_state().gyro()[1] = 0.8;
    msg1.imu_state().gyro()[2] = 0.9;
    msg1.bms_state().cycle() = 5;
    msg1.bms_state().soc() = 50.0;
    msg1.bms_state().remaining_cap() = 80.0;
    msg1.bms_state().total_cap() = 100.0;
    msg1.bms_state().current() = 10.0;
    msg1.bms_state().voltage() = 20.0;

    msg2.acc()[0] = 1.1;
    msg2.acc()[1] = 1.2;
    msg2.acc()[2] = 1.3;
    msg2.rpy()[0] = 1.4;
    msg2.rpy()[1] = 1.5;
    msg2.rpy()[2] = 1.6;
    msg2.gyro()[0] = 1.7;
    msg2.gyro()[1] = 1.8;
    msg2.gyro()[2] = 1.9;

    msg3.cycle() = 15;
    msg3.soc() = 150.0;
    msg3.remaining_cap() = 180.0;
    msg3.total_cap() = 1100.0;
    msg3.current() = 110.0;
    msg3.voltage() = 120.0;

    constexpr size_t MSG_NUMS = 1000;
    constexpr size_t SLEEP_TIME = 1;  // seconds
    for (size_t i = 0; i < MSG_NUMS; ++i) {
        pub1->write(&msg1);
        pub2->write(&msg2);
        pub3->write(&msg3);
        fmt::print("Write message\n");
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }
    pub1->closeChannel();
    pub2->closeChannel();
    pub3->closeChannel();
    return 0;
}
