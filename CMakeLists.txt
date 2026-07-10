cmake_minimum_required(VERSION 3.20)

project(CyaeghaEngine CXX C RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: keep incremental .obj in build dir; avoid /MP conflicts with /FS via godot_common.
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
    string(APPEND CMAKE_C_FLAGS " /FS")
    string(APPEND CMAKE_CXX_FLAGS " /FS")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(GODOT_EDITOR_BUILD "Build editor" ON)
option(GODOT_DEV_BUILD "Developer build (.dev suffix, DEV_ENABLED)" OFF)
option(GODOT_FORCE_BOOTSTRAP "Re-run codegen and source scan on configure" OFF)
set(GODOT_PLATFORM "windows" CACHE STRING "Target platform")
set(GODOT_TARGET "editor" CACHE STRING "editor | template_release | template_debug")
set(GODOT_ARCH "x86_64" CACHE STRING "Target architecture")
set(GODOT_DISABLED_MODULES "" CACHE STRING "Comma-separated disabled modules")

if(NOT GODOT_TARGET STREQUAL "editor")
    set(GODOT_EDITOR_BUILD OFF)
endif()

# ---------------------------------------------------------------------------
# Bootstrap (incremental: skipped when scripts unchanged)
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(GodotBootstrap)
godot_run_bootstrap()

include("${CMAKE_SOURCE_DIR}/cmake/generated/godot_build.cmake")
include(GodotCommon)

# ---------------------------------------------------------------------------
# Platform + targets
# ---------------------------------------------------------------------------
if(GODOT_PLATFORM STREQUAL "windows")
    include(GodotPlatformWindows)
    godot_configure_windows_msvc()
endif()

include(GodotBuild)
godot_build_targets()

message(STATUS "Cyaegha native CMake build configured")
message(STATUS "  Binary: bin/${GODOT_BINARY_NAME}.exe")
message(STATUS "  Incremental: cmake --build ${CMAKE_BINARY_DIR} --config Debug")
