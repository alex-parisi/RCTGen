include(FetchContent)

# Pin to a released tag. GoogleTest's CMake honors the parent project's
# C++ standard, so cxx_std_23 propagates correctly.
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
)
# Don't install GoogleTest alongside the project.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# Match the project's static-runtime build on MSVC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(rct2_ride_gen_unit_tests
        rct2-ride-gen/Constants.t.cpp
        rct2-ride-gen/SpriteRenderer.t.cpp
        rct2-ride-gen/ProjectLoader.t.cpp
)
target_link_libraries(rct2_ride_gen_unit_tests PRIVATE
        rct2_ride_gen_lib
        GTest::gtest_main
)

gtest_discover_tests(rct2_ride_gen_unit_tests)

add_executable(rctgen_util_unit_tests
        util/Json.t.cpp
        util/Logging.t.cpp
)
# Json.t.cpp uses Constants.hpp from rct2-ride-gen as test fixtures (enum/flag
# name arrays); rct2_ride_gen_lib transitively links RctgenUtil.
target_link_libraries(rctgen_util_unit_tests PRIVATE
        rct2_ride_gen_lib
        GTest::gtest_main
)

gtest_discover_tests(rctgen_util_unit_tests)

add_executable(iso_render_unit_tests
        iso-render/VectorMath.t.cpp
        iso-render/Palette.t.cpp
        iso-render/Image.t.cpp
)
target_link_libraries(iso_render_unit_tests PRIVATE
        IsoRender
        GTest::gtest_main
)

gtest_discover_tests(iso_render_unit_tests)

add_executable(track_render_unit_tests
        track-render/Constants.t.cpp
        track-render/TrackSections.t.cpp
)
target_link_libraries(track_render_unit_tests PRIVATE
        TrackCommon
        GTest::gtest_main
)

gtest_discover_tests(track_render_unit_tests)
