# ========================================================================
# All Options:
#
# * BUILD_RELEASE             -> ON: build in Release mode, OFF: build in Debug mode
# * BUILD_COMM_SHARED          -> ON: build COMM as shared library (.so/.dll), OFF: static library (.a/.lib)
# * BUILD_PYTHON_BINDING      -> ON: build Python binding module, OFF: skip
# * BUILD_TEST                -> ON: build test targets
# * ENABLE_DOXYGEN            -> ON: use doxygen to generate documentation
# * BUILD_EXAMPLE             -> ON: build example targets
# * SANITIZERS_ADDRESS_ON     -> ON: enable AddressSanitizer (ASan) for memory error detection
# * SANITIZERS_THREAD_ON      -> ON: enable ThreadSanitizer (TSan) for data race detection
# * SANITIZERS_UNDEFINED_ON   -> ON: enable UndefinedBehaviorSanitizer (UBSan)
# * PY_INSTALL                -> ON: install Python module after build
# ========================================================================
# Build option
option(BUILD_RELEASE "Build Type of Release" ON)
# SHARED(ON) or STATIC(OFF)
option(BUILD_COMM_SHARED "Build Comm Type of Shared" OFF)
option(BUILD_PYTHON_BINDING "Build Python Binding" ON)
option(BUILD_TEST "Build Test" ON)
# Compiling together with Examples requires modifying cmake to add_build_type
option(BUILD_EXAMPLE "Build Example" OFF)
option(ENABLE_DOXYGEN "Enable Doxygen" OFF)
# Debug option
option(SANITIZERS_ADDRESS_ON "Enable AddressSanitizer" OFF)
option(SANITIZERS_THREAD_ON "Enable ThreadSanitizer" OFF)
option(SANITIZERS_UNDEFINED_ON "Enable UndefinedBehaviorSanitizer" ON)
# Install option
option(PY_INSTALL "Install for python" ON)

message(
    STATUS "OPTIONS\n"
           "===================================\n"
           "Build Release: ${BUILD_RELEASE}\n"
           "Build Comm Shared: ${BUILD_COMM_SHARED}\n"
           "Build Python Binding: ${BUILD_PYTHON_BINDING}\n"
           "Build Example: ${BUILD_EXAMPLE}\n"
           "Enable Doxygen: ${ENABLE_DOXYGEN}\n"
           "Build Test: ${BUILD_TEST}\n"
           "Sanitizers Address: ${SANITIZERS_ADDRESS_ON}\n"
           "Sanitizers Thread: ${SANITIZERS_THREAD_ON}\n"
           "Sanitizers Undefined: ${SANITIZERS_UNDEFINED_ON}\n"
           "Install for python: ${PY_INSTALL}\n"
           "===================================")
