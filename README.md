# Joyson Robot SDK V2

🤖 Joyson Robot SDK V2

This is the SDK developed and used by Joyson Robot, designed to provide developers with convenient access to robot APIs. It offers publisher/subscriber communication and corresponding RPC services based on [Fast-DDS](https://github.com/eProsima/Fast-DDS), along with C++ and Python interfaces. Testing is conducted using [Catch2](https://github.com/catchorg/Catch2). The SDK is developed in C++17, built with CMake, and targeted for Linux systems.

This repository contains:

1. The SDK implementation [header files](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/include) and [source files](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/src).

2. The [IDL files](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/idl) used by the SDK and the [scripts](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/third_party/scripts/ddsgen) to generate code from them.

3. The [test code](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/tests) for this SDK.


4. The [Python interface implementation](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/python) for this SDK.

> [Chinese version Readme](http://192.168.124.192:3000/TanJiachun/joysonrobot_sdk_v2/src/branch/main/README.zh-CN.md)

---

## Table of Contents

- [Joyson Robot SDK V2](#joyson-robot-sdk-v2)
  - [Table of Contents](#table-of-contents)
  - [Background](#background)
  - [Dependencies](#dependencies)
  - [Usage](#usage)
    - [Use jsrsdk in another project](#use-jsrsdk-in-another-project)
    - [Code Generator](#code-generator)
    - [Communicating with ROS2](#communicating-with-ros2)
  - [Examples](#examples)
  - [License](#license)

---

## Background

`Joyson Robot SDK V2` is inspired by the [Booster Robotics SDK](https://github.com/BoosterRobotics/booster_robotics_sdk) project, with extended implementations, modifications, and improvements to its modules and features. It aims to deliver a similar development experience.

> **Note: Internal source code and backend code should not be publicly distributed.**

---

## Dependencies

To build this project, install the following dependencies (example shown using `apt`):

> **Note: tinyxml version must be 9.0.0**

```sh
sudo apt-get update
sudo apt-get install cmake build-essential python3-dev python3-pip
sudo apt-get install pybind11-dev libtinyxml2-dev libssl-dev
pip3 install pybind11-stubgen
```

---

## Usage

This section explains how to build and install the SDK.

```
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j$(nproc)
```

Run the tests in the build/tests directory to ensure everything works (excluding hardware-specific tests).

Install the SDK to your system:

```sh
cd build && sudo make install
```

> The Python interface requires the SDK to be installed system-wide. And you need to set the environment variable export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

### Use jsrsdk in another project
- If jsrsdk is already installed
Add the following to CMakeLists.txt:

```cmake
find_package(jsrsdk REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC /usr/local/include/jsrsdk)
target_link_libraries(${TARGET_NAME} PRIVATE jsrsdk fastcdr fastrtps)
# Or put it before <TARGET>
# include_directories(BEFORE /usr/local/include/jsrsdk)
# link_libraries(jsrsdk fastcdr fastrtps)
```

- If using a prebuilt jsrsdk directly
Add the following to CMakeLists.txt:

```cmake
set(JSRSDK_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsrsdk/lib/")
file(GLOB JSRSDK_LIB_SOURCES CONFIGURE_DEPENDS "${JSRSDK_LIB}/lib*.so*")
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsrsdk/include)
target_link_libraries(<TARGET> PRIVATE ${JSRSDK_LIB_SOURCES})
```
Place the prebuilt jsrsdk library files into the third_party/jsrsdk/lib folder, and the prebuilt jsrsdk header files into third_party/jsrsdk/include (including header and library files of third-party dependencies).

Then jsrsdk can be used. The Python interface must be written in python/binding.cpp and jsrsdk must be installed to use it.

### Code Generator

To use the generator, first read about [writing IDL files](https://fast-dds.docs.eprosima.com/en/2.6.x/fastddsgen/dataTypes/dataTypes.html). Place your `.idl` files in the `idl/` directory of this project, and run the following commands:

```sh
sudo apt-get install openjdk17-jdk
cd third_party/scripts/ddsgen
# Build the generator tool
./gradlew assemble
# Replace <FILE_NAME> with your IDL file (without path)
# The script automatically searches for the IDL file in the project’s idl/ folder.
# Use * to generate code for all IDL files at once.
# Generated files are placed in idl/genfiles/.
# Recommended: Use the CMake parameter to regenerate automatically. The generated files will be copied to include/idl and src/idl.
# ./run.sh <FILE_NAME>
```

Rebuild the project with regenerated IDL

```
mkdir -p build && cd build
cmake .. -DREGEN_IDL=ON
make -j$(nproc)
```

> Note: Ensure all IDL files are correctly written and use English identifiers only to avoid generation errors.


### Communicating with ROS2

Please ensure that [ROS2](https://docs.ros.org/en/humble/Installation.html) is installed before using the SDK's topic communication interface. Also, ensure that the msg interface in ROS2 matches the IDL file.

> Note: Topic names must follow the ROS2 naming convention and be named "rt/xxx", where rt is the abbreviation for ROS TOPIC and xxx is the name of the topic discovered by ROS2.

---

## Examples

Here are some basic examples (see examples/cpp for details):

1. Publisher:

```cpp
#include "idl/little_robot/ImuState.hpp"
#include "robot/channel/channel_publisher.hpp"

namespace jrc = jsr::robot::channel;
using namespace jsr::msg;

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    // Must be an IDL-generated message type
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

2. Subscriber:

```cpp
#include "idl/little_robot/ImuState.hpp"
#include "robot/channel/channel_subscriber.hpp"

namespace jrc = jsr::robot::channel;
using namespace jsr::msg;

void Handler(const void* msg_ptr) {
    const auto* msg = static_cast<const ImuState*>(msg_ptr);
    fmt::print("Received message:header-> {},{},{} | {},{},{} | {},{},{}\n",
               msg->acc()[0], msg->acc()[1], msg->acc()[2],
               msg->gyro()[0], msg->gyro()[1], msg->gyro()[2],
               msg->rpy()[0], msg->rpy()[1], msg->rpy()[2]);
}

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    // domain_id, type, and topic name must match to communicate
    jrc::ChannelSubscriber<ImuState> sub("imu", Handler);
    sub.InitChannel();
    for (size_t i = 0; i < 100; ++i) {
        fmt::print("Sub | {} : Sleep 1 second\n", sub.GetChannelName());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
```

3. RPC Server:

```cpp
#include "robot/rpc/error.hpp"
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
#include "robot/rpc/rpc_client.hpp"
#include "robot/rpc/rpc_server.hpp"

namespace jrc = jsr::robot::channel;
namespace jrr = jsr::robot::rpc;

enum class LocoApiId {
    // Define your API IDs here
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
            // Handle each API ID
            default:
                response.SetHeader(jrr::ResponseHeader(jrr::RpcStatusCodeInvalid));
                response.SetBody("Get unknown request");
                break;
        }

        fmt::print("[Server] | Get Request with api id={}, body={}\n", api_id, response.GetBody());
        return response;
    }
};

int main() {
    jrc::ChannelFactory::Instance()->Init(0);
    auto server = std::make_shared<LocoServer>();
    server->Init("loco/server");
    // Run for 100 seconds
    for (size_t i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}

```

4. RPC Client:

```cpp
#include "robot/rpc/request.hpp"
#include "robot/rpc/response.hpp"
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

        // This will send request and wait for response
        client->SendApiRequest(req);
    }
}
```


## License

This project is licensed under the Apache License, Version 2.0. See the LICENSE file for details.

This project uses the following third-party libraries:
- booster_robotics_sdk (Apache License 2.0)
- fastDDS (Apache License 2.0)
- pybind11 (BSD 3-Clause License)
- pybind11-stubgen (MIT License)
- catch2（Boost Software License 1.0）
- fmt (MIT License)