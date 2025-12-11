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
set(jsrcomm_installed_path /usr/local/)
include_directories(BEFORE "${jsrcomm_installed_path}/include/jsrcomm")
set(libs "${jsrcomm_installed_path}/lib/libjsrcomm.a" "${jsrcomm_installed_path}/lib/libjsrcomm_idl.a"
         "${jsrcomm_installed_path}/lib/libfastdds.so" "${jsrcomm_installed_path}/lib/libfastcdr.so")

file(GLOB examples "${CMAKE_SOURCE_DIR}/cpp/*.cpp")
foreach(example ${examples})
    get_filename_component(example_name ${example} NAME_WE)
    add_executable(${example_name} ${example})
    target_link_libraries(${example_name} PRIVATE ${libs})
endforeach()
