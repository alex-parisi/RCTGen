include(FetchContent)

# Pin to a released tag. GoogleTest's CMake honors the parent project's
# C++ standard, so cxx_std_23 propagates correctly.
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
# Don't install GoogleTest alongside the project.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# Match the project's static-runtime build on MSVC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(rct2_ride_gen_unit_tests
    test_constants.cpp
    test_json.cpp
    test_logging.cpp
    test_sprite_renderer.cpp
    test_project_loader.cpp
)
target_link_libraries(rct2_ride_gen_unit_tests PRIVATE
    rct2_ride_gen_lib
    GTest::gtest_main
)

gtest_discover_tests(rct2_ride_gen_unit_tests)
