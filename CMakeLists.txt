cmake_minimum_required(VERSION 3.21)

project(RCT2RideGen
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

# Use the static MSVC runtime to match the historic build configuration.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Place binaries and libraries in a per-config directory under the build tree.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# Treat ours as the active VS startup project when generating Visual Studio
# solutions.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Helper interface library carrying compile features/options shared by every
# target in this project. Keeps target definitions below short.
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
# IsoRender — core rendering static library
# ---------------------------------------------------------------------------

add_library(IsoRender STATIC
    libIsoRender/src/vectormath.cpp
    libIsoRender/src/renderer.cpp
    libIsoRender/src/model.cpp
    libIsoRender/src/palette.cpp
    libIsoRender/src/raytrace.cpp
    libIsoRender/src/image.cpp
    libIsoRender/src/pack.cpp
)

target_include_directories(IsoRender PUBLIC libIsoRender/src)

# Public deps are those whose headers leak through IsoRender's own public
# headers (raytrace.h pulls embree, image.h pulls libpng). Everything else is
# an implementation detail.
target_link_libraries(IsoRender
    PUBLIC
        rctgen_common_flags
        PNG::PNG
        embree
    PRIVATE
        ZLIB::ZLIB
        assimp::assimp
)

# ---------------------------------------------------------------------------
# Executables
# ---------------------------------------------------------------------------

add_executable(TestIsoRender libIsoRender/src/main.cpp)
target_link_libraries(TestIsoRender PRIVATE IsoRender jansson::jansson)

add_executable(makevehicle
    RCT2RideGen/src/main.cpp
    RCT2RideGen/src/project.cpp
)
target_link_libraries(makevehicle PRIVATE
    IsoRender
    jansson::jansson
    libzip::zip
)
# Make the examples directory available next to the binary at build time.
add_custom_command(TARGET makevehicle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:makevehicle>/examples"
    VERBATIM
)

add_executable(maketrack
    TrackRender/src/main.cpp
    TrackRender/src/track.cpp
    TrackRender/src/sprites.cpp
    TrackRender/src/track_sections.cpp
    TrackRender/src/mask.cpp
)
target_link_libraries(maketrack PRIVATE
    IsoRender
    jansson::jansson
    libzip::zip
)

add_executable(subposition
    TrackRender/src/subposition.cpp
    TrackRender/src/sprites.cpp
    TrackRender/src/track_sections.cpp
)
target_link_libraries(subposition PRIVATE
    IsoRender
    jansson::jansson
    libzip::zip
)

# Pick a reasonable default VS startup project without overriding any user
# choice already in the cache.
get_directory_property(_vs_startup DIRECTORY "${PROJECT_SOURCE_DIR}" DEFINITION VS_STARTUP_PROJECT)
if(NOT _vs_startup)
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT makevehicle)
endif()
