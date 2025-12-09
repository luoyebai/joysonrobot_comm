find_program(DOXYGEN_EXECUTABLE doxygen REQUIRED)

set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_BINARY_DIR}/docs/Doxyfile)
set(DOC_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs)

file(MAKE_DIRECTORY ${DOC_OUTPUT_DIR})

set(PROJECT_NAME_DOC "Joyson Robot SDK Version 2")
set(PROJECT_BRIEF "Joysonrobot公司提供的机器人SDK封装架构")
set(DOXYGEN_INPUT_DIRS "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src")
set(DOXYGEN_EXCLUDES "${CMAKE_SOURCE_DIR}/tests" "${CMAKE_SOURCE_DIR}/third_party" "${CMAKE_SOURCE_DIR}/idl")

string(REPLACE ";" " " DOXYGEN_INPUT "${DOXYGEN_INPUT_DIRS}")
string(REPLACE ";" " " DOXYGEN_EXCLUDE "${DOXYGEN_EXCLUDES}")
string(REPLACE ";" " " PROJECT_BRIEF "${PROJECT_BRIEF}")
string(REPLACE ";" " " PROJECT_NAME_DOC "${PROJECT_NAME_DOC}")
string(REPLACE ";" " " PROJECT_NUMBER "${PROJECT_VERSION}")

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_target(
    doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    WORKING_DIRECTORY ${DOC_OUTPUT_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM)

add_custom_target(
    doc-clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOC_OUTPUT_DIR}
    COMMENT "Cleaning Doxygen docs")
