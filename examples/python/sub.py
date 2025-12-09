import sys
import time
from jsrsdk_python import ChannelFactory, LRLowStateSubscriber

SLEEP_TIME = 1


def handler(low_state_msg):
    try:
        print("----Received message----")

        print(f"serial motor count: {len(low_state_msg.motor_state_serial)}")
        print(
            f"parallel motor count: {len(low_state_msg.motor_state_parallel)}")
        bms_state = low_state_msg.bms_state
        print(
            # f"  bms:{bms_state.voltage: .2f}V {bms_state.current: .2f}A {bms_state.soc: .2f}% cycle:{bms_state.cycle}"
            f"  bms:{bms_state.voltage}V {bms_state.current:}A {bms_state.soc: .2f}% cycle:{bms_state.cycle}"
        )
        imu_state = low_state_msg.imu_state
        print(
            f"  imu rpy: {imu_state.rpy[0]}, {imu_state.rpy[1]}, {imu_state.rpy[2]}\n"
            f"  imu gyro: {imu_state.gyro[0]}, {imu_state.gyro[1]}, {imu_state.gyro[2]}\n"
            f"  imu acc: {imu_state.acc[0]}, {imu_state.acc[1]}, {imu_state.acc[2]}"
        )
        for i, motor in enumerate(low_state_msg.motor_state_serial):
            print(
                f"  serial motor {i+1}: {motor.q: .2f} {motor.dq: .2f}, "
                f"{motor.ddq: .2f}, {motor.tau_est: .2f}"
            )
        for i, motor in enumerate(low_state_msg.motor_state_parallel):
            print(
                f"  parallel motor {i+1}: {motor.q:.2f} {motor.dq:.2f}, "
                f"{motor.ddq:.2f}, {motor.tau_est:.2f}"
            )
        print("----Received over----")
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        time.sleep(0.1)


def main():
    ChannelFactory.Instance().Init(0)
    channel_subscriber = LRLowStateSubscriber(handler)
    channel_subscriber.InitChannel()
    print("init handler")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nReceived interrupt, shutting down...", file=sys.stderr)
    except BrokenPipeError:
        print("Pipe broken, shutting down...", file=sys.stderr)
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
    finally:
        channel_subscriber.CloseChannel()
        print("Channel closed", file=sys.stderr)


if __name__ == "__main__":
    main()
