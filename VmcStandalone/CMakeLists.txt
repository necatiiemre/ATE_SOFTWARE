cmake_minimum_required(VERSION 3.10)

project(VmcStandalone
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SOURCES
    src/main.cpp
    src/SystemCommand.cpp
    src/SshClient.cpp
    src/CumulusVmc.cpp
    src/VmcTestRunner.cpp
    src/Logger.cpp
)

add_executable(vmcStandalone ${SOURCES})

target_include_directories(vmcStandalone PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(vmcStandalone PRIVATE -Wall -Wextra)

# Logger runs a background thread (stdout/stderr -> console + log file)
find_package(Threads REQUIRED)
target_link_libraries(vmcStandalone PRIVATE Threads::Threads)

set_target_properties(vmcStandalone PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ==================== Runtime asset: VMC interfaces file ====================
# The interfaces file lives INSIDE this project
# (VmcStandalone/CumulusInterfaces/VMC/interfaces) - the app is fully
# independent of the rest of the repository.
#
# configure_file(COPYONLY) puts a copy next to the binary so build/bin/ is
# self-contained, and registers the source as a configure dependency: when
# the file is edited, the next `cmake --build` re-copies it automatically.
set(VMC_INTERFACES_SRC ${CMAKE_SOURCE_DIR}/CumulusInterfaces/VMC/interfaces)
if(NOT EXISTS ${VMC_INTERFACES_SRC})
    message(FATAL_ERROR "VMC interfaces file not found: ${VMC_INTERFACES_SRC}")
endif()
configure_file(${VMC_INTERFACES_SRC}
               ${CMAKE_BINARY_DIR}/bin/CumulusInterfaces/VMC/interfaces
               COPYONLY)

# ==================== vmc_test: compiled ON THE SERVER ====================
# Same idea as prepare_release.sh for dpdk_vmc: the sources in vmc_test/ are
# copied to the ATE server, built there with its own Makefile, and the binary
# is fetched back to build/bin/vmc_test/. vmcStandalone later deploys that
# prebuilt binary to the server and runs it (step 2).
#
# Runs as part of a normal build (`make`), but only when something in
# vmc_test/, the script or ServerConfig.h has changed. Rebuild on demand with
# `make vmc_test`. Needs the server to be reachable; build without it with
#   cmake -DVMC_TEST_SERVER_BUILD=OFF ..
option(VMC_TEST_SERVER_BUILD "Build vmc_test on the ATE server during the build" ON)

set(VMC_TEST_SRC_DIR   ${CMAKE_SOURCE_DIR}/vmc_test)
set(VMC_TEST_OUT_DIR   ${CMAKE_BINARY_DIR}/bin/vmc_test)
set(VMC_TEST_SCRIPT    ${CMAKE_SOURCE_DIR}/scripts/build_vmc_test_on_server.sh)
set(SERVER_CONFIG_H    ${CMAKE_SOURCE_DIR}/include/ServerConfig.h)

if(VMC_TEST_SERVER_BUILD)
    file(GLOB_RECURSE VMC_TEST_SOURCES CONFIGURE_DEPENDS
        ${VMC_TEST_SRC_DIR}/*.c
        ${VMC_TEST_SRC_DIR}/*.h
    )

    add_custom_command(
        OUTPUT  ${VMC_TEST_OUT_DIR}/vmc_test
        COMMAND bash ${VMC_TEST_SCRIPT} ${VMC_TEST_SRC_DIR} ${VMC_TEST_OUT_DIR} ${SERVER_CONFIG_H}
        DEPENDS ${VMC_TEST_SOURCES}
                ${VMC_TEST_SRC_DIR}/Makefile
                ${VMC_TEST_SCRIPT}
                ${SERVER_CONFIG_H}
        COMMENT "Building vmc_test on the ATE server and fetching the binary"
        USES_TERMINAL
        VERBATIM
    )
    add_custom_target(vmc_test ALL DEPENDS ${VMC_TEST_OUT_DIR}/vmc_test)
else()
    message(STATUS "VMC_TEST_SERVER_BUILD=OFF: vmc_test will NOT be built on the server")
endif()
