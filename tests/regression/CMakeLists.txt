# Regression tests: re-render every example project with the current
# makevehicle binary and verify each output PNG is byte-identical to the
# golden image checked into tests/regression/golden/.
#
# Goldens were captured at commit edea916 (the base of this branch). The
# 19 commits on top are refactors that must not change render output.

set(_regression_golden_dir "${CMAKE_CURRENT_SOURCE_DIR}/golden")
set(_regression_driver     "${CMAKE_CURRENT_SOURCE_DIR}/run_one.cmake")

set(_regression_projects
    alpine
    hybrid
    single_rail
)

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
    )
endforeach()
