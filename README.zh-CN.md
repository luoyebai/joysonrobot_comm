# Joyson Robot SDK V2

🤖 Joyson Robot SDK V2

Joyson Robot 公司所使用的机器人SDK封装代码，供开发人员使用。提供了基于[Fast-DDS](https://github.com/eProsima/Fast-DDS)的订阅者发布者通讯以及对应的RPC服务调用，同时提供了相应的C++和Python接口，测试系统为[Catch2](https://github.com/catchorg/Catch2)，使用C++17进行开发，使用CMake进行构建，开发平台为Linux。

本仓库主要包含以下内容：

1. 该SDK实现对应的[头文件](https://github.com/luoyebai/joysonrobot_sdk2_user/tree/main/include)和[源文件](https://github.com/luoyebai/joysonrobot_sdk2_user/tree/main/src)。

2. 该SDK使用的[IDL文件](https://github.com/luoyebai/joysonrobot_sdk2_user/tree/main/idl)和[根据IDL生成代码的脚本](https://github.com/luoyebai/joysonrobot_sdk2_user/tree/main/third_party/scripts/ddsgen)。

3. 该SDK对应的[测试代码](https://github.com/luoyebai/joysonrobot_sdk2_user/tree/main/tests)。

4. 该SDK对应的[Python接口实现](https://github.com/luoyebai/joysonrobot_sdk2_user/tree/main/python)。

## 内容列表

- [Joyson Robot SDK V2](#joyson-robot-sdk-v2)
  - [内容列表](#内容列表)
  - [背景](#背景)
  - [安装依赖](#安装依赖)
  - [使用说明](#使用说明)
    - [在其他项目中使用jsrsdk](#在其他项目中使用jsrsdk)
    - [生成器](#生成器)
    - [与ROS2通讯](#与ros2通讯)
  - [示例](#示例)
  - [许可证](#许可证)

## 背景

`Joyson Robot SDK V2` 是类 [Booster Robotics SDK](https://github.com/BoosterRobotics/booster_robotics_sdk) 项目的SDK，其中增加了对应的源文件的可能实现，并且增加和修改部分模块和功能，最终实现类似的开发效果。

## 安装依赖

本项目需要安装以下依赖（以apt包管理为例）:
> **tinyxml需要为9.0.0版本**

```sh
sudo apt-get update
sudo apt-get install cmake build-essential python3-dev python3-pip
sudo apt-get install pybind11-dev libtinyxml2-dev libssl-dev
pip3 install pybind11-stubgen
```
---

## 使用说明

这里是开发该SDK的说明。需要先进行编译测试。

```sh
mkdir build && cd build
cmake ..
make -j$(nproc)
```

然后运行build/tests下的测试代码，确保所有测试（硬件相关测试除外）通过。

安装SDK到系统。

```sh
cd build && sudo make install
```

> 使用Python接口需要将SDK安装到系统，并且需要设置环境变量export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

### 在其他项目中使用jsrsdk

- 已经安装jsrsdk的情况
在CMakeLists.txt中添加以下内容：
```cmake
find_package(jsrsdk REQUIRED)
# 此处应与安装路径对应
target_include_directories(<TARGET> PUBLIC /usr/local/include/jsrsdk)
target_link_libraries(<TARGET> PRIVATE jsrsdk fastcdr fastrtps)
# 或者放在<TARGET>之前
# include_directories(BEFORE /usr/local/include/jsrsdk)
# link_libraries(jsrsdk fastcdr fastrtps)
```

- 直接使用编译好的jsrsdk的情况
在CMakeLists.txt中添加以下内容：
```cmake
set(JSRSDK_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsrsdk/lib/")
file(GLOB JSRSDK_LIB_SOURCES CONFIGURE_DEPENDS "${JSRSDK_LIB}/lib*.so*")
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsrsdk/include)
target_link_libraries(<TARGET> PRIVATE ${JSRSDK_LIB_SOURCES})
```
将编译好的jsrsdk库文件放入third_party/jsrsdk/lib文件夹中，将编译好的jsrsdk头文件放入third_party/jsrsdk/include文件夹中（包括第三方依赖的头文件和库文件）。

然后就可以使用jsrsdk了。**其中python接口需要在python/binding.cpp文件中编写，并且必须安装jsrsdk才能使用。**

### 生成器

想要使用生成器的话，请先了解[IDL文件的编写](https://fast-dds.docs.eprosima.com/en/3.4.x/fastddsgen/dataTypes/dataTypes.html)，将编写IDL文件放入该项目下的idl文件夹，然后使用以下命令生成工具。

```sh
# **通常来说你不需要重新编译该工具**
sudo apt-get install openjdk17-jdk
cd third_party/scripts/ddsgen
# 运行成功后会得到一个生成工具
./gradlew assemble
# 生成
./run.sh <FILE_NAME>
```

通常情况，直接通过 cmake 编译即可，注意不要使用文件夹分层，这可能导致脚本无法找到idl文件
如果需要分层，可以通过软链接的方式将文件分层管理。当然，你可以通过脚本来直接生成文件。

- 直接使用脚本生成
  - 你可以直接运行脚本用来生成，输入文件名即可，脚本会自动查找本项目 idl 文件夹中的 idl 文件，
可用 * 来执行全部生成。
  - 生成后的文件在项目的 idl/genfiles 文件夹下，一份 idl 生成的内容包括 4个文件，分别是 *.hpp、*.cpp、*.ipp、*.pybind.ipp 。
- 使用 cmake 命令进行生成
  - cmake 会自动调用脚本进行生成，请注意不要在idl文件夹下随意添加后缀为.idl的文件。
  - 生成的 hpp 和 pybind.ipp 会被分别拷贝到项目文件夹 include/idl 和 python/idl 下
  - 脚本路径发生变化的情况下，需要在 cmake/genidl.cmake 修改配置。
  - 如需修改拷贝文件路径也请在 cmake/genidl.cmake 中修改参数配置。

> 你可以通过引入 hpp 文件来使用 idl 数据类型, pybind.ipp 文件可以被编译到你需要使用的 python 绑定中，请注意文件引用顺序，pybind.ipp 对应的头文件需要提前引入。

添加/修改 idl 文件后，需要重新运行 cmake 生成文件。

```sh
cmake ..
make -j$(nproc)
```

idl 生成正确的情况下，会编译得到一份 link 文件，其命名在 CMakeLists.txt 中通过调用 cmake 函数指定。

### 与ROS2通讯

请先确保安装了[ROS2](https://docs.ros.org/en/humble/Installation.html)，然后使用该SDK的话题通讯接口。同时请在ROS2中安装与idl文件相同接口的msg接口。

> 注意：话题名需要遵循 ROS2 命名规则，命名为"rt/xxx"，rt为ROS TOPIC的缩写，xxx为 ROS2 发现的话题名。

---

## 示例

以下是几个基本示例（详细请查阅examples/cpp文件夹）：

1. 发布者：
```cpp
#include "idl/little_robot/ImuState.hpp"
#include "robot/channel/channel_publisher.hpp"

namespace jrc = jsr::robot::channel;
using namespace jsr::msg;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
	// 必须为idl生成的消息类型
    jrc::ChannelPublisher<ImuState> pub("imu");
    pub.InitChannel();
	auto msg = ImuState();
    msg.gyro({0, 0, 0});
    msg.acc({0, 0, 0});
    msg.rpy({0, 0, 0});
    for (size_t i = 0; i < 100000; ++i) {
        pub.Write(&msg);
        fmt::print("Pub | {} | : Write message\n", pub.GetChannelName());
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }
    return 0;
}
```

2. 订阅者：
```cpp
#include "idl/little_robot/ImuState.hpp"
#include "robot/channel/channel_subscriber.hpp"

namespace jrc = jsr::robot::channel;
using namespace jsr::msg;

void Handler(const void* msg_ptr) {
    const auto* msg = static_cast<const ImuState*>(msg_ptr);
    fmt::print("Received message:header-> {},{},{} | {},{},{} | {},{},{}\n", msg->acc()[0], msg->acc()[1],
               msg->acc()[2], msg->gyro()[0], msg->gyro()[1], msg->gyro()[2], msg->rpy()[0], msg->rpy()[1],
               msg->rpy()[2]);
    return;
}

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
	// domain_id和类型和话题名相同即可通讯
    jrc::ChannelSubscriber<ImuState> sub("imu", Handler);
    sub.InitChannel();
    for (size_t i = 0; i < 100; ++i) {
        fmt::print("Sub | {} :Sleep 1 second\n", sub.GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
```

3. Rpc服务端：

```cpp
// RPC
#include "robot/rpc/error.hpp"
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC SERVER
#include "robot/rpc/rpc_client.hpp"
#include "robot/rpc/rpc_server.hpp"

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;

enum class LocoApiId {
	// ......
};

class LocoServer : public jrr::RpcServer {
   public:
    LocoServer() = default;
    ~LocoServer() = default;

   private:
    jrr::Response HandleRequest(const jrr::Request& request) override {
        auto api_id = request.GetHeader().GetApiId();
        auto response = jrr::Response();
        switch (static_cast<LocoApiId>(api_id)) {
			// 对所有的服务进行响应
			// 详细见代码：
            default:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeInvalid));
                response.SetBody("Get unknown request");
				break;
        }
        fmt::print("[Server]| Get Request with api id={},body={}\n", api_id, response.GetBody());
        return response;
    }
};

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto server = std::make_shared<LocoServer>();
    server->Init("loco/server");
    // 运行100s
    for (size_t i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
	return 0;
}
```

4. Rpc客户端：

```cpp
// RPC
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
// RPC CLIENT
#include "robot/rpc/rpc_client.hpp"

namespace jrc = jsr::robot::channel;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto client = std::make_shared<jsr::robot::rpc::RpcClient>();
    client->Init("loco/server");
    int api = 0;
    jsr::robot::rpc::Request req;
    jsr::robot::rpc::RequestHeader header;
    while (true) {
        fmt::print("\nEnter API number: ");
        std::cin >> api;
        header.SetApiId(api);
        req.SetHeader(header);
        req.SetBody("This is a message");
		// 这里能得到响应Response
        client->SendApiRequest(req);
    }
}
```

## 许可证

本项目采用 Apache 许可证 2.0 版本授权，具体条款详见 LICENSE 文件。
本项目使用了以下第三方库：
- booster_robotics_sdk（Apache 许可证 2.0）
- fastDDS（Apache 许可证 2.0）
- pybind11（BSD 3-Clause 许可证）
- pybind11-stubgen（MIT 许可证）
- catch2（Boost Software 许可证 1.0）
- fmt（MIT 许可证）