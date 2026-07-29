cmake_minimum_required(VERSION 3.10)

project(mainSoftware
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/UnitManager.cpp
    src/DeviceManager.cpp
    src/TdkLambdaPsu.cpp
    src/Dtn.cpp
    src/Hsn.cpp
    src/Cmc.cpp
    src/Mmc.cpp
    src/Vmc.cpp
    src/Utils.cpp
    src/SerialPort.cpp
    src/SystemCommand.cpp
    src/Server.cpp
    src/SSHDeployer.cpp
    src/CumulusHelper.cpp
    src/SerialTimeForwarder.cpp
    src/ReportManager.cpp
    src/SafeShutdown.cpp
    src/PsuTelemetryPublisher.cpp
    src/AppMode.cpp
    src/FirmwareUpdaterRunner.cpp
    src/FlickerDetectionRunner.cpp
)

# pthread for std::thread (PsuTelemetryPublisher background loop)
find_package(Threads REQUIRED)

# ==================== Component versions (build-time) ====================
# Each component keeps its version in a single source of truth header named
# after the component:
#   <component>/include/<Name>Version.h  ->  #define <Name>Version "x.y.z"
#   (dpdk variants use their unit name: DTNIRSWVersion, CMCVersion, VMCVersion)
# We read those values here and inject them as compile definitions so the
# report header can print them at runtime. This is the same value that
# prepare_release.sh stamps into the release package - kept in one place.
function(read_component_version out_var header)
    set(_ver "unknown")
    if(EXISTS "${header}")
        file(STRINGS "${header}" _line REGEX "^[ \t]*#define[ \t]+[A-Za-z_][A-Za-z0-9_]*Version[ \t]+\"")
        if(_line)
            string(REGEX REPLACE ".*\"([^\"]*)\".*" "\\1" _ver "${_line}")
        endif()
    endif()
    set(${out_var} "${_ver}" PARENT_SCOPE)
endfunction()

# MainSoftware's own version + the per-unit DPDK variants it deploys:
#   DTN -> dpdk, CMC -> dpdk_cmc, VMC -> dpdk_vmc (MMC/HSN run no DPDK app)
read_component_version(MAINSOFTWARE_VERSION ${CMAKE_SOURCE_DIR}/include/MainSoftwareVersion.h)
read_component_version(DPDK_DTN_VERSION     ${CMAKE_SOURCE_DIR}/../dpdk/include/DTNIRSWVersion.h)
read_component_version(DPDK_CMC_VERSION     ${CMAKE_SOURCE_DIR}/../dpdk_cmc/include/CMCVersion.h)
read_component_version(DPDK_VMC_VERSION     ${CMAKE_SOURCE_DIR}/../dpdk_vmc/include/VMCVersion.h)

message(STATUS "MainSoftware version: ${MAINSOFTWARE_VERSION}")
message(STATUS "DPDK versions - DTN:${DPDK_DTN_VERSION} CMC:${DPDK_CMC_VERSION} VMC:${DPDK_VMC_VERSION}")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MAINSOFTWARE_VERSION="${MAINSOFTWARE_VERSION}"
    DPDK_DTN_VERSION="${DPDK_DTN_VERSION}"
    DPDK_CMC_VERSION="${DPDK_CMC_VERSION}"
    DPDK_VMC_VERSION="${DPDK_VMC_VERSION}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Output setting for build directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
