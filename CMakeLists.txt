# ===============
# basic setting #
# ===============
# Minimal cmake version required
cmake_minimum_required(VERSION 3.5...${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

# Project setting
set(TARGET_NAME jsrcomm)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(cmake_path ${CMAKE_SOURCE_DIR}/cmake)
# setting
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "runtime")

project(
    ${TARGET_NAME}
    VERSION 2.1.0
    LANGUAGES CXX)

# option
include(${cmake_path}/option.cmake)
# common
include(${cmake_path}/common.cmake)

# ==========
# get arch #
# ==========
detect_arch_and_set_third_party_lib(ARCH_THIRD LIB_THIRD)
message(STATUS "ARCH_THIRD: ${ARCH_THIRD}")
message(STATUS "LIB_THIRD: ${LIB_THIRD}")
if(NOT EXISTS "${LIB_THIRD}")
    message(FATAL_ERROR "Required third party library directory not found")
endif()

# =============================
# include & add_libray & link #
# =============================

# src
file(GLOB SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp)

# include
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)

# libs
file(GLOB LIB_SOURCECS CONFIGURE_DEPENDS "${LIB_THIRD}/lib*.so*" "${LIB_THIRD}/*.a")
check_third_party_libs()

if(BUILD_COMM_SHARED)
    add_library(${TARGET_NAME} SHARED ${SOURCES})
else()
    add_library(${TARGET_NAME} STATIC ${SOURCES})
endif()

if(ENABLE_DDS)
    set(TARGET_GEN ${TARGET_NAME}_idl)
    add_compile_definitions(ENABLE_DDS)
    if(ENABLE_GRPC)
        set(TARGET_GEN ${TARGET_NAME}_idl ${TARGET_NAME}_proto)
        add_compile_definitions(ENABLE_GRPC)
    endif()
elseif(ENABLE_GRPC)
    set(TARGET_GEN ${TARGET_NAME}_proto)
    add_compile_definitions(ENABLE_GRPC)
else()
    message(FATAL_ERROR "ENABLE_DDS or ENABLE_GRPC must be set")
endif()

target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_GEN} ${LIB_SOURCECS})
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

# ==============
# set compiler #
# ==============
# Use other compliler options
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -fPIC)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    if(BUILD_RELEASE)
        set(CMAKE_BUILD_TYPE
            Release
            CACHE STRING "Build type release" FORCE)
    else()
        set(CMAKE_BUILD_TYPE
            Debug
            CACHE STRING "Build type debug" FORCE)
    endif()
endif()

if(NOT BUILD_RELEASE)
    enable_sanitizers(${TARGET_NAME})
endif()

# =========
# doxygen #
# =========
if(ENABLE_DOXYGEN)
    include(${cmake_path}/doxygen.cmake)
endif()

# ==============
# generate idl #
# ==============

if(ENABLE_DDS)
    set(IDL_DEPS ${LIB_SOURCECS})
    include(${cmake_path}/genidl.cmake)
    file(GLOB_RECURSE idl_files "${CMAKE_SOURCE_DIR}/idl/*.idl")
    gendds_for_idl(${TARGET_NAME}_idl ${idl_files})
endif()

if(ENABLE_GRPC)
    file(GLOB_RECURSE proto_files "${CMAKE_SOURCE_DIR}/proto/*.proto")
    include(${cmake_path}/genproto.cmake)
    gengrpc_for_proto(${TARGET_NAME}_proto ${proto_files})
endif()

# ================
# examples build #
# ================
if(BUILD_EXAMPLE)
    add_subdirectory(examples)
endif()

# =============
# tests build #
# =============
if(BUILD_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()

# =========
# install #
# =========
include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX
    "/opt/joysonrobot"
    CACHE PATH "Install path prefix" FORCE)

target_include_directories(
    ${TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/include>
           $<INSTALL_INTERFACE:include/${TARGET_NAME}>)

# header
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_NAME})
# third party header
install(DIRECTORY third_party/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_NAME})
# target libs
install(
    TARGETS ${TARGET_NAME} ${TARGET_GEN}
    EXPORT ${TARGET_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # dynamic
)
# third_party .so libraries
install(FILES ${LIB_SOURCECS} DESTINATION ${CMAKE_INSTALL_LIBDIR})

# cmake config
install(
    EXPORT ${TARGET_NAME}
    FILE ${TARGET_NAME}Config.cmake
    NAMESPACE ${TARGET_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME})