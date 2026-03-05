# If it is just for compilation, you can only keep the following line add_build_type(example cpp examples LIBS)

# The following is an example of cmake calling jsrcomm

# Minimal cmake version required
cmake_minimum_required(VERSION 3.5...${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
project(jsrcomm_example)

# setting
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use installed path
set(joysonrobot_installed_path /opt/joysonrobot)
list(APPEND CMAKE_PREFIX_PATH ${joysonrobot_installed_path})
find_package(Protobuf REQUIRED)
find_package(jsrcomm REQUIRED)

file(GLOB examples "${CMAKE_SOURCE_DIR}/cpp/*.cpp")
foreach(example ${examples})
    get_filename_component(example_name ${example} NAME_WE)
    add_executable(${example_name} ${example})
    target_include_directories(${example_name} PRIVATE ${jsrcomm_INCLUDE_DIRS})
    target_link_libraries(${example_name} PRIVATE ${libs} jsrcomm::jsrcomm jsrcomm::jsrcomm_idl jsrcomm::jsrcomm_proto)
endforeach()
