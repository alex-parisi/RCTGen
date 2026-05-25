cmake_minimum_required(VERSION 3.25)

# Auto-detect vcpkg without requiring the toolchain to be passed via preset or
# -D on every invocation. If VCPKG_ROOT is set in the environment to a real
# directory and the user hasn't already selected a toolchain (e.g. via
# -DCMAKE_TOOLCHAIN_FILE=...), point CMake at vcpkg's toolchain. Has to
# happen before project() to take effect during system/compiler detection.
# Builds with brew/apt-installed system deps just skip this block and rely
# on find_package finding them directly. We test STREQUAL "" rather than
# DEFINED because some IDE environments (CLion) export VCPKG_ROOT as the
# empty string when no vcpkg is configured.
if(NOT "$ENV{VCPKG_ROOT}" STREQUAL "" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain (auto-detected from VCPKG_ROOT)")
endif()

project(src/rct2-ride-gen
    VERSION 22.02
    LANGUAGES CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-tree builds are not supported. Run CMake from a separate directory, "
        "e.g. `cmake -B build`."
    )
endif()

# ---------------------------------------------------------------------------
# Project-wide defaults
# ---------------------------------------------------------------------------

# Match the historic static-runtime build on MSVC. CMP0091 is automatically
# NEW at cmake_minimum_required >= 3.15, so no explicit cmake_policy needed.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Per-config output directories under the build tree.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# Folder-style target grouping in IDE generators.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Emit compile_commands.json for clangd and other tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Shared compile features/options applied to every project target.
add_library(rctgen_common_flags INTERFACE)
target_compile_features(rctgen_common_flags INTERFACE cxx_std_23)

# ---------------------------------------------------------------------------
# Dependencies (all resolved via vcpkg by default)
# ---------------------------------------------------------------------------

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(embree 4 CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(jansson REQUIRED IMPORTED_TARGET jansson)
add_library(jansson::jansson ALIAS PkgConfig::jansson)

# ---------------------------------------------------------------------------
# Subprojects
# ---------------------------------------------------------------------------

add_subdirectory(src/iso-render)
add_subdirectory(src/rct2-ride-gen)
add_subdirectory(src/track-render)

# Default startup project for Visual Studio.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT makevehicle)
