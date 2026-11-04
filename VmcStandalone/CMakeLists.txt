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
)

add_executable(vmcStandalone ${SOURCES})

target_include_directories(vmcStandalone PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(vmcStandalone PRIVATE -Wall -Wextra)

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
