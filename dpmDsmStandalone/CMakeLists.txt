cmake_minimum_required(VERSION 3.10)

project(dpmDsmStandalone
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

add_executable(dpmDsmStandalone ${SOURCES})

target_include_directories(dpmDsmStandalone PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(dpmDsmStandalone PRIVATE -Wall -Wextra)

# Logger runs a background thread (stdout/stderr -> console + log file)
find_package(Threads REQUIRED)
target_link_libraries(dpmDsmStandalone PRIVATE Threads::Threads)

set_target_properties(dpmDsmStandalone PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ==================== Runtime assets: DPM / DSM interfaces files ====================
# The interfaces files live INSIDE this project
# (dpmDsmStandalone/CumulusInterfaces/<DPM|DSM>/interfaces) - the app is fully
# independent of the rest of the repository. The unit selected at start-up
# picks which one is deployed (see include/UnitConfig.h).
#
# configure_file(COPYONLY) puts a copy next to the binary so build/bin/ is
# self-contained, and registers the source as a configure dependency: when
# a file is edited, the next `cmake --build` re-copies it automatically.
foreach(UNIT DPM DSM)
    set(UNIT_INTERFACES_SRC ${CMAKE_SOURCE_DIR}/CumulusInterfaces/${UNIT}/interfaces)
    if(NOT EXISTS ${UNIT_INTERFACES_SRC})
        message(FATAL_ERROR "${UNIT} interfaces file not found: ${UNIT_INTERFACES_SRC}")
    endif()
    configure_file(${UNIT_INTERFACES_SRC}
                   ${CMAKE_BINARY_DIR}/bin/CumulusInterfaces/${UNIT}/interfaces
                   COPYONLY)
endforeach()

# ==================== dpm_dsm_test: compiled ON THE SERVER ====================
# Same idea as prepare_release.sh for dpdk_vmc: the sources in dpm_dsm_test/ are
# copied to the ATE server, built there with its own Makefile, and the binary
# is fetched back to build/bin/dpm_dsm_test/. dpmDsmStandalone later deploys that
# prebuilt binary to the server and runs it (step 2).
#
# Runs as part of a normal build (`make`), but only when something in
# dpm_dsm_test/, the script or ServerConfig.h has changed. Rebuild on demand with
# `make dpm_dsm_test`. Needs the server to be reachable; build without it with
#   cmake -DDPM_DSM_TEST_SERVER_BUILD=OFF ..
option(DPM_DSM_TEST_SERVER_BUILD "Build dpm_dsm_test on the ATE server during the build" ON)

set(VMC_TEST_SRC_DIR   ${CMAKE_SOURCE_DIR}/dpm_dsm_test)
set(VMC_TEST_OUT_DIR   ${CMAKE_BINARY_DIR}/bin/dpm_dsm_test)
set(VMC_TEST_SCRIPT    ${CMAKE_SOURCE_DIR}/scripts/build_dpm_dsm_test_on_server.sh)
set(SERVER_CONFIG_H    ${CMAKE_SOURCE_DIR}/include/ServerConfig.h)

if(DPM_DSM_TEST_SERVER_BUILD)
    file(GLOB_RECURSE VMC_TEST_SOURCES CONFIGURE_DEPENDS
        ${VMC_TEST_SRC_DIR}/*.c
        ${VMC_TEST_SRC_DIR}/*.h
    )

    add_custom_command(
        OUTPUT  ${VMC_TEST_OUT_DIR}/dpm_dsm_test
        COMMAND bash ${VMC_TEST_SCRIPT} ${VMC_TEST_SRC_DIR} ${VMC_TEST_OUT_DIR} ${SERVER_CONFIG_H}
        DEPENDS ${VMC_TEST_SOURCES}
                ${VMC_TEST_SRC_DIR}/Makefile
                ${VMC_TEST_SCRIPT}
                ${SERVER_CONFIG_H}
        COMMENT "Building dpm_dsm_test on the ATE server and fetching the binary"
        USES_TERMINAL
        VERBATIM
    )
    add_custom_target(dpm_dsm_test ALL DEPENDS ${VMC_TEST_OUT_DIR}/dpm_dsm_test)
else()
    message(STATUS "DPM_DSM_TEST_SERVER_BUILD=OFF: dpm_dsm_test will NOT be built on the server")
endif()
