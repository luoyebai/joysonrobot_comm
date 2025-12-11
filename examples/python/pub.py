import time
from jsrcomm_python import ChannelFactory, LRLowCmdPublisher
from jsrcomm_python import LowCmd, LowCmdType, MotorCmd
from jsrcomm_python import LRJointCnt, LRJointIndex

SLEEP_TIME = 1


def main():
    ChannelFactory.Instance().Init(0)
    channel_publisher = LRLowCmdPublisher()
    channel_publisher.InitChannel()
    motor_cmds = [MotorCmd() for _ in range(LRJointCnt)]

    while True:
        low_cmd = LowCmd()
        low_cmd.cmd_type = LowCmdType.SERIAL
        low_cmd.motor_cmd = motor_cmds
        for i in range(LRJointCnt):
            low_cmd.motor_cmd[i].q = 0.0
            low_cmd.motor_cmd[i].dq = 0.0
            low_cmd.motor_cmd[i].tau = 0.0
            low_cmd.motor_cmd[i].kp = 0.0
            low_cmd.motor_cmd[i].kd = 0.0
            low_cmd.motor_cmd[i].weight = 0.0
            if i == LRJointIndex.CrankDownLeft.value:
                low_cmd.motor_cmd[i].q = 0.1
                low_cmd.motor_cmd[i].dq = 0.0
                low_cmd.motor_cmd[i].tau = 0.0
                low_cmd.motor_cmd[i].kp = 4.0
                low_cmd.motor_cmd[i].kd = 1.0
                low_cmd.motor_cmd[i].weight = 1.0

        channel_publisher.Write(low_cmd)
        print("Publish LowCmd")
        time.sleep(SLEEP_TIME)


if __name__ == "__main__":
    main()
