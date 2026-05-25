# Regression tests: re-render every example project with the current
# makevehicle binary and verify each output PNG is byte-identical to the
# golden image checked into tests/regression/golden/.
#
# Goldens were captured at commit edea916 (the base of this branch). The
# 19 commits on top are refactors that must not change render output.
#
# Pixel-exact comparison is build-config-dependent: the compiler emits
# different float instructions at -O0 vs -O2 (fp-contraction, inlining, FMA
# selection, etc.), which changes LSBs in the dithered output. The goldens
# were captured from a Release build, so these tests only run when ctest is
# invoked against a Release configuration. With a single-config generator
# (Ninja) configured for Debug, the tests are skipped at configure time.
# With multi-config (Xcode/VS), the per-test `CONFIGURATIONS Release`
# property handles it.

set(_regression_golden_dir "${CMAKE_CURRENT_SOURCE_DIR}/golden")
set(_regression_driver     "${CMAKE_CURRENT_SOURCE_DIR}/run_one.cmake")

set(_regression_projects
    alpine
    hybrid
    single_rail
)

# Single-config generators: skip the suite entirely if we're not Release.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS
        "Skipping pixel-exact regression tests because CMAKE_BUILD_TYPE is "
        "'${CMAKE_BUILD_TYPE}' (goldens were captured from a Release build).")
    return()
endif()

foreach(proj IN LISTS _regression_projects)
    add_test(
        NAME regression_${proj}
        COMMAND ${CMAKE_COMMAND}
            -D "BINARY=$<TARGET_FILE:makevehicle>"
            -D "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            -D "GOLDEN_DIR=${_regression_golden_dir}"
            -D "WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/work/${proj}"
            -D "PROJECT=${proj}"
            -P "${_regression_driver}"
    )
    set_tests_properties(regression_${proj} PROPERTIES
        LABELS regression
        TIMEOUT 600
        CONFIGURATIONS Release
    )
endforeach()
