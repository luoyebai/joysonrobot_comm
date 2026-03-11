function(register_tests)
    cmake_parse_arguments(ARG "" "PREFIX" "TEST_EXECUTABLES" ${ARGN})

    if(NOT ARG_TEST_EXECUTABLES)
        message(FATAL_ERROR "No test executables provided to register_tests()")
    endif()

    foreach(test_exe ${ARG_TEST_EXECUTABLES})
        if(ARG_PREFIX)
            set(test_name "${ARG_PREFIX}-${test_exe}")
        else()
            set(test_name "${test_exe}")
        endif()
        add_test(NAME ${test_name} COMMAND ${test_exe})
        message(STATUS "Registered test: ${test_name} -> ${test_exe}")
    endforeach()
endfunction()

add_build_type(
    test
    .
    tests
    LIBS
    ${TARGET_NAME}
    ${TARGET_GEN}
    SOURCES
    test_tools/catch_amalgamated.cpp)

set(TEST_EXECUTABLES unit_tests benchmark_tests)

register_tests(TEST_EXECUTABLES ${TEST_EXECUTABLES} PREFIX ${TARGET_NAME})
