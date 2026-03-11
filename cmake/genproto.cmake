list(APPEND CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
find_package(Protobuf REQUIRED)

# ========================================================================
# Function:
#
# * gengrpc_for_proto(IDL_NAMES...)
# * Automatically generates DDS sources from IDL files located in ${CMAKE_SOURCE_DIR}/idl/ directory.
#
# Params:
#
# * TARGET_NAME -> The name of generated IDL static link
# * IDL_NAMES   -> List of IDL file names (without path and without .idl)
#
# Behavior:
#
# * Searches for ${CMAKE_SOURCE_DIR}/idl/<name>.idl
# * Generates C++ source and header files for DDS
# ========================================================================
function(gengrpc_for_proto IDL_LIB_NAME)
    set(PROTO_FILES ${ARGN})
    set(PROTO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/${TARGET_NAME}/proto)

    message(STATUS "Use proto:")

    foreach(proto ${PROTO_FILES})
        get_filename_component(proto_name ${proto} NAME_WE)
        message(STATUS "* ${proto}")
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h
                   ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc
                   ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
            COMMAND protoc ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I
                    ${CMAKE_CURRENT_SOURCE_DIR}/proto --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin ${proto}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h
                    ${PROTO_INCLUDE_DIR}/${proto_name}.pb.h
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
                    ${PROTO_INCLUDE_DIR}/${proto_name}.grpc.pb.h
            DEPENDS ${proto})

        list(APPEND GENERATED_PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc
             ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc)
    endforeach()
    add_library(${IDL_LIB_NAME} ${GENERATED_PROTO_SRCS})
    target_include_directories(${IDL_LIB_NAME} PRIVATE ${GRPC_INCLUDE_DIRS})
    target_link_libraries(${IDL_LIB_NAME} PUBLIC ${GRPC_LIBRARIES} grpc++_reflection protobuf::libprotobuf)
endfunction()
