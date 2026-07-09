cmake_minimum_required(VERSION 3.20)

project(CyaeghaEngine CXX C RC)

if(MSVC)
    add_compile_options(/FS /std:c++17)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(GODOT_EDITOR_BUILD "Build editor" ON)
option(GODOT_DEV_BUILD "Developer build (.dev suffix, DEV_ENABLED)" OFF)
set(GODOT_PLATFORM "windows" CACHE STRING "Target platform")
set(GODOT_TARGET "editor" CACHE STRING "editor | template_release | template_debug")
set(GODOT_ARCH "x86_64" CACHE STRING "Target architecture")
set(GODOT_DISABLED_MODULES "" CACHE STRING "Comma-separated disabled modules")

if(NOT GODOT_EDITOR_BUILD OR NOT GODOT_TARGET STREQUAL "editor")
    set(GODOT_EDITOR_BUILD OFF)
endif()

# ---------------------------------------------------------------------------
# Bootstrap: codegen + source collection (no SCons)
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(_bootstrap_args
    --platform "${GODOT_PLATFORM}"
    --target "${GODOT_TARGET}"
    --arch "${GODOT_ARCH}"
    --platform-exporters "${GODOT_PLATFORM}"
    --platform-apis "${GODOT_PLATFORM}"
)
if(GODOT_DEV_BUILD)
    list(APPEND _bootstrap_args --dev-build)
endif()
if(GODOT_DISABLED_MODULES)
    list(APPEND _bootstrap_args --disabled-modules "${GODOT_DISABLED_MODULES}")
endif()

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/cmake/scripts/godot_bootstrap.py" ${_bootstrap_args}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _bootstrap_result
    COMMAND_ECHO STDOUT
)
if(NOT _bootstrap_result EQUAL 0)
    message(FATAL_ERROR "godot_bootstrap.py failed with code ${_bootstrap_result}")
endif()

include("${CMAKE_SOURCE_DIR}/cmake/generated/godot_build.cmake")

# ---------------------------------------------------------------------------
# Platform configuration
# ---------------------------------------------------------------------------
if(GODOT_PLATFORM STREQUAL "windows")
    include(GodotPlatformWindows)
    godot_configure_windows_msvc()
endif()

include(GodotBuild)
godot_build_targets()

message(STATUS "Cyaegha native CMake build configured")
message(STATUS "  Binary: bin/${GODOT_BINARY_NAME}.exe")
