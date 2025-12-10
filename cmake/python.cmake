# ========================================================================
# Function:
#
# * add_python_binding(target_name python_sources py_install)
# * Automatically build a Python module from C++ sources using pybind11.
#
# Parameters:
#
# * target_name -> Base C++ target name to link against
# * python_sources -> pybind11 sources
# * py_install -> Optional boolean (ON/OFF) to install the module
#
# Requirements:
#
# * Python 3 (Interpreter + Development.Module)
# * pybind11 (CONFIG mode)
# * pybind11-stubgen (for .pyi generation)
#
# ========================================================================
function(add_python_binding target_name python_sources py_install)

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        message(STATUS "CMake >= 3.18: Use Development.Module")
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    else()
        message(STATUS "CMake < 3.18: Use Development")
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    endif()

    message(STATUS "Python3 executable: ${Python3_EXECUTABLE}")

    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_paths()['purelib'])"
        OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    include_directories(${Python3_INCLUDE_DIRS})
    find_package(pybind11 CONFIG REQUIRED)

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
        message(STATUS "CMake >= 3.21: Use WITH_SOABI")
        python3_add_library(${target_name}_python MODULE ${python_sources} WITH_SOABI)
    else()
        message(STATUS "CMake < 3.21: Without WITH_SOABI")
        python3_add_library(${target_name}_python MODULE ${python_sources})
    endif()

    target_link_libraries(${target_name}_python PRIVATE ${target_name} ${target_name}_idl pybind11::module)

    target_compile_definitions(${target_name}_python PRIVATE VERSION_INFO=${PROJECT_VERSION})

    find_program(PYBIND11_STUBGEN_EXECUTABLE pybind11-stubgen)
    if(NOT PYBIND11_STUBGEN_EXECUTABLE)
        message(FATAL_ERROR "pybind11-stubgen not found")
    endif()

    add_custom_command(
        TARGET ${target_name}_python
        POST_BUILD
        COMMAND PYTHONPATH=${CMAKE_SOURCE_DIR}/build:/${PYTHONPATH} pybind11-stubgen -o ${CMAKE_SOURCE_DIR}/build
                ${target_name}_python)

    if(${py_install})
        install(TARGETS ${target_name}_python LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
        install(FILES ${CMAKE_SOURCE_DIR}/build/${target_name}_python.pyi DESTINATION ${PYTHON_SITE_PACKAGES})
    endif()

endfunction()
