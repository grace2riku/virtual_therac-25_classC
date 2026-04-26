cmake_minimum_required(VERSION 3.28)

# vcpkg toolchain integration must be set BEFORE project() is called.
# Users typically set CMAKE_TOOLCHAIN_FILE via -DCMAKE_TOOLCHAIN_FILE=...
# or via CMakePresets.json.

project(th25_ctrl
    VERSION 0.2.0
    DESCRIPTION "Therac-25 Virtual Control Software (TH25-CTRL) — IEC 62304 Class C learning project"
    LANGUAGES CXX
)

# C++ standard: C++20 (SDP-TH25-001 §6.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clang-tidy / IDE integration.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type when not specified by a preset.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Strict warnings (SDP §6.1 / RCM-001 / RCM-008).
# Per-target compile options are added at the target level to avoid affecting SOUP.
set(TH25_STRICT_WARNINGS
    -Wall -Wextra -Wpedantic
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    -Wcast-align -Wunused -Woverloaded-virtual
    -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion -Wformat=2
)

# Sanitizer option: passed via CMakePresets.json or -DTH25_SANITIZER=...
set(TH25_SANITIZER "" CACHE STRING "Sanitizer to enable: address, undefined, thread, or empty")
set_property(CACHE TH25_SANITIZER PROPERTY STRINGS "" "address" "undefined" "thread")

if(TH25_SANITIZER)
    message(STATUS "TH25_SANITIZER enabled: ${TH25_SANITIZER}")
    add_compile_options(-fsanitize=${TH25_SANITIZER} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${TH25_SANITIZER})
endif()

# Project-wide include directory for placeholder headers.
include_directories(${PROJECT_SOURCE_DIR}/src)

# Sub-projects (Inc.1 placeholders; populated in Step 17+).
add_subdirectory(src/th25_ctrl)
add_subdirectory(src/th25_sim)

# Tests (Step 17+ で UT を本格実装、本 Step 14 ではスモークテストのみ).
# vcpkg toolchain 不在の開発環境ではテストツリーを無効化できる:
#   cmake -B build -DTH25_BUILD_TESTS=OFF
option(TH25_BUILD_TESTS "Build the test tree (requires GoogleTest via vcpkg)" ON)
if(TH25_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "TH25_BUILD_TESTS=OFF — テストツリーをスキップします (CI では ON 必須).")
endif()
