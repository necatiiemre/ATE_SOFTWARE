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
# Single source of truth stays in <repo>/CumulusInterfaces/VMC/interfaces
# (the same file MainSoftware deploys). It is copied next to the binary on
# every build so build/bin/ is self-contained and can be moved anywhere.
set(VMC_INTERFACES_SRC ${CMAKE_SOURCE_DIR}/../CumulusInterfaces/VMC/interfaces)
if(EXISTS ${VMC_INTERFACES_SRC})
    add_custom_command(TARGET vmcStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:vmcStandalone>/CumulusInterfaces/VMC
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${VMC_INTERFACES_SRC}
                $<TARGET_FILE_DIR:vmcStandalone>/CumulusInterfaces/VMC/interfaces
        COMMENT "Copying VMC Cumulus interfaces file next to vmcStandalone"
    )
else()
    message(WARNING "VMC interfaces file not found: ${VMC_INTERFACES_SRC} "
                    "- pass it at runtime with --interfaces")
endif()
