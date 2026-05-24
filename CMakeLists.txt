cmake_minimum_required(VERSION 3.25)

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
target_compile_features(rctgen_common_flags INTERFACE cxx_std_17)
if(NOT MSVC)
    # Legacy code relies on narrowing conversions that C++11 made ill-formed.
    target_compile_options(rctgen_common_flags INTERFACE -Wno-c++11-narrowing)
endif()

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
