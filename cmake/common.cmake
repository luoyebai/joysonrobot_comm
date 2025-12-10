# ========================================================================
# Function:
#
# * detect_arch_and_set_third_party_lib(OUT_ARCH_VAR OUT_LIB_DIR_VAR)
# * Detects the current system architecture and sets the corresponding third-party library directory path.
#
# Parameters:
#
# * OUT_ARCH_VAR -> Output variable name to store the detected architecture
# * OUT_LIB_DIR_VAR   -> Output variable name to store the resolved third-party library path based on architecture
#
# Example:
#
# * detect_arch_and_set_third_party_lib(ARCH_VAR LIB_DIR_VAR)
#
# Behavior:
#
# * Converts architecture string to lowercase
# * Validates that the third-party directory exists
# * Terminates CMake generation if the directory is missing
# ========================================================================
function(detect_arch_and_set_third_party_lib OUT_ARCH_VAR OUT_LIB_DIR)
    # Detect architecture
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
    string(TOLOWER "${ARCH}" ARCH)
    message(STATUS "Detected architecture: ${ARCH}")

    # Set third party library path
    set(THIRD_PARTY_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/${ARCH}")

    if(NOT EXISTS "${THIRD_PARTY_LIB}")
        message(FATAL_ERROR "Required third party library directory not found: ${THIRD_PARTY_LIB}")
    endif()

    # Return variables to caller
    set(${OUT_ARCH_VAR}
        "${ARCH}"
        PARENT_SCOPE)
    set(${OUT_LIB_DIR}
        "${THIRD_PARTY_LIB}"
        PARENT_SCOPE)
endfunction()

# ========================================================================
# Function:
#
# * enable_sanitizers(target)
# * Enables runtime Sanitizers for the specified target, All supported options are configured in the cmake/option.cmake
#   file.
#
# Supported sanitizer flags:
#
# * SANITIZERS_ADDRESS_ON   -> AddressSanitizer (ASan) detects memory errors/leaks
# * SANITIZERS_THREAD_ON    -> ThreadSanitizer (TSan) detects data races
# * SANITIZERS_UNDEFINED_ON -> UBSan detects undefined behavior
#
# Notes:
#
# * ASan and TSan cannot be enabled at the same time; CMake will stop with an error
# * Only supported with Clang and GCC compilers
# * MSVC does not support Sanitizers; a warning will be shown
# ========================================================================
function(enable_sanitizers target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(sanitizers_list "")
        if(SANITIZERS_ADDRESS_ON)
            list(APPEND sanitizers_list "address")
        endif()
        if(SANITIZERS_THREAD_ON)
            list(APPEND sanitizers_list "thread")
        endif()
        if(SANITIZERS_UNDEFINED_ON)
            list(APPEND sanitizers_list "undefined")
        endif()
        list(FIND sanitizers_list "address" idx_addr)
        list(FIND sanitizers_list "thread" idx_thread)
        if(idx_addr GREATER -1 AND idx_thread GREATER -1)
            message(FATAL_ERROR "Cannot enable both AddressSanitizer and ThreadSanitizer at the same time.")
        endif()
        if(sanitizers_list)
            string(JOIN "," sanitizer_flags ${sanitizers_list})
            message(STATUS "Enabling sanitizers (${sanitizer_flags}) for target: ${target}")
            target_compile_options(${target} PRIVATE -fsanitize=${sanitizer_flags} -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=${sanitizer_flags} -fno-omit-frame-pointer)
        else()
            message(STATUS "No sanitizers enabled for target: ${target}")
        endif()
    elseif(MSVC)
        message(WARNING "Sanitizers are not supported with MSVC.")
    endif()
endfunction()

# ========================================================================
# Function:
#
# * add_build_type(type_name dir_name output)
# * Automatically build all .cpp files in the specified directory into executables
#
# Parameters:
#
# * type_name -> Key used to categorize and store target names (global property)
# * dir_name  -> Directory containing source files; each .cpp generates one executable
# * output    -> Output directory for the generated executables
#
# Optional arguments (passed as lists):
#
# * LIBS      -> Additional libraries to link against
# * SOURCES   -> Additional source files to compile
#
# Example:
#
# * add_build_type(test . tests SOURCES test/catch_amalgamated.cpp)
#
# Global property:
#
# * All generated target names will be stored in ${type_name}_targets
# ========================================================================
function(add_build_type type_name dir_name output)
    cmake_parse_arguments(ADD_BUILD_TYPE "" "" "LIBS;SOURCES" ${ARGN})
    file(GLOB source_files CONFIGURE_DEPENDS "${dir_name}/*.cpp")
    foreach(src ${source_files})
        get_filename_component(target_name ${src} NAME_WE)
        add_executable(${target_name} ${src} ${ADD_BUILD_TYPE_SOURCES})
        if(NOT BUILD_RELEASE)
            enable_sanitizers(${target_name})
        endif()
        target_link_options(${target_name} PRIVATE "-Wl,--no-as-needed")
        target_link_libraries(${target_name} PRIVATE ${TARGET_NAME} ${LIB_SOURCES} ${ADD_BUILD_TYPE_LIBS})
        set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${output})
        set_property(GLOBAL APPEND PROPERTY ${type_name}_targets ${target_name})
    endforeach()
endfunction()

# ========================================================================
# Function:
#
# * check_third_party_libs()
# * Checks for missing dependencies in third party libraries
#
# Global property:
#
# * LIB_SOURCECS       -> List of third party libraries to check
# ========================================================================
function(check_third_party_libs)
    message(STATUS "Checking shared libs in: LIB_THIRD variable")
    foreach(lib_path ${LIB_SOURCECS})
        get_filename_component(fname ${lib_path} NAME)
        # match lib so
        if(lib_path MATCHES "\\.so(\\..*)?$")
            message(STATUS "→ Checking shared library: ${fname}")
            execute_process(
                COMMAND ldd ${lib_path}
                RESULT_VARIABLE LDD_RESULT
                OUTPUT_VARIABLE LDD_OUTPUT
                ERROR_VARIABLE LDD_ERROR)

            if(NOT LDD_RESULT EQUAL 0)
                message(FATAL_ERROR "ldd failed for ${fname}\n${LDD_OUTPUT}\n${LDD_ERROR}")
            endif()
            string(REGEX REPLACE "\n" ";" LDD_LINES "${LDD_OUTPUT}")
            foreach(line ${LDD_LINES})
                string(FIND "${line}" "not found" NOT_FOUND_POS)
                if(NOT_FOUND_POS GREATER -1)
                    string(REGEX MATCH "[^[:space:]]+\\.so(\\.[0-9]+)*" MISSING_SO "${line}")
                    list(FIND LIB_SOURCECS "${MISSING_SO}" FOUND_POS)
                    message(STATUS ${LIB_SOURCECS})
                    if(FOUNT_POS EQUAL -1)
                        message(FATAL_ERROR "Missing dependency detected in ${fname}:\n${MISSING_SO}")
                    else()
                        message(STATUS "Warning: missing ${line}, but ignored because it's in third_party")
                    endif()
                endif()
            endforeach()
            message(STATUS "   OK: ${fname}")
        else()
            message(STATUS "→ Skipping static library: ${fname}")
        endif()
    endforeach()
endfunction()
