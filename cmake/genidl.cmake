# ========================================================================
# Function:
#
# * gendds_for_idl(IDL_NAMES...)
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
function(gendds_for_idl IDL_LIB_NAME)
    set(IDL_FILES ${ARGN})

    find_program(_DDSGEN_SCRIPT ${CMAKE_SOURCE_DIR}/third_party/scripts/ddsgen/run.sh)
    message(STATUS "RUN SCRIPT ${_DDSGEN_SCRIPT}")

    message(STATUS "Use idl:")
    foreach(IDL_FILE_NAME ${IDL_FILES})
        message(STATUS "* ${IDL_FILE_NAME}")
    endforeach()

    set(IDL_GEN_ROOT ${CMAKE_SOURCE_DIR}/idl/genfiles)
    set(IDL_GEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/idl)
    set(PYBIND_SRC_DIR ${CMAKE_SOURCE_DIR}/python/idl)

    set(GENERATED_SRCS "")
    set(GENERATED_HDRS "")

    file(MAKE_DIRECTORY ${IDL_GEN_ROOT} ${IDL_GEN_INCLUDE_DIR})

    set(idl_src)
    set(idl_hdr)
    # Extract idl names without extension
    foreach(idl ${IDL_FILES})
        get_filename_component(IDL_NAME ${idl} NAME_WE)
        set(idl_src "${IDL_GEN_ROOT}/${IDL_NAME}.cpp")
        set(idl_ipp "${IDL_GEN_ROOT}/${IDL_NAME}.ipp")
        set(idl_hdr "${IDL_GEN_INCLUDE_DIR}/${IDL_NAME}.hpp")
        set(idl_pybind "${PYBIND_SRC_DIR}/${IDL_NAME}.pybind.ipp")

        list(APPEND GENERATED_SRCS ${idl_src})
        list(APPEND GENERATED_HDRS ${idl_hdr})

        add_custom_command(
            OUTPUT ${idl_src} ${idl_ipp} ${idl_hdr}
            COMMAND ${_DDSGEN_SCRIPT} ${IDL_NAME} ${BUILD_PYTHON_BINDING}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IDL_GEN_ROOT}/${IDL_NAME}.hpp ${idl_hdr}
            COMMENT "Generating DDS code for ${IDL_NAME}"
            DEPENDS ${idl})

        if(BUILD_PYTHON_BINDING)
            add_custom_command(
                OUTPUT ${idl_pybind}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${IDL_GEN_ROOT}/${IDL_NAME}.pybind.ipp ${idl_pybind}
                COMMENT "Generating DDS pybind code for ${IDL_NAME}"
                DEPENDS ${idl_src} ${idl_hdr})
        endif()
    endforeach()

    add_library(${IDL_LIB_NAME} ${GENERATED_SRCS} ${GENERATED_HDRS})
    target_link_libraries(${IDL_LIB_NAME} PUBLIC ${IDL_DEPS})

endfunction()
