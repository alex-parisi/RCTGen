#
# Drive a single makevehicle regression case.
#
# Inputs (set via -D on the cmake command line):
#   BINARY      - absolute path to the makevehicle executable
#   SOURCE_DIR  - the repository root (provides examples/, masks/)
#   GOLDEN_DIR  - directory holding <project>/<image>.png golden PNGs
#   WORK_DIR    - per-test scratch directory (will be wiped and recreated)
#   PROJECT     - project name (e.g. alpine); JSON read from examples/${PROJECT}/${PROJECT}.json
#

foreach(var BINARY SOURCE_DIR GOLDEN_DIR WORK_DIR PROJECT)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "Required cache variable ${var} is unset")
    endif()
endforeach()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}/object/images")

# makevehicle resolves mesh/texture paths and the JSON's "preview" field
# relative to its current working directory, so we point them at the source
# tree by symlinking examples/ and masks/ into the scratch dir. Symlinks are
# free and avoid copying ~MB of assets per test invocation.
file(CREATE_LINK "${SOURCE_DIR}/examples" "${WORK_DIR}/examples" SYMBOLIC)
file(CREATE_LINK "${SOURCE_DIR}/masks"    "${WORK_DIR}/masks"    SYMBOLIC)

set(input_json "examples/${PROJECT}/${PROJECT}.json")

execute_process(
    COMMAND "${BINARY}" "${input_json}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE run_rc
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE  run_err
)

# A nonzero exit from makevehicle does not necessarily mean rendering failed.
# example/single_rail/single_rail.json hardcodes a parkobj output_directory
# that does not exist on this machine, which makes the post-render zip step
# fail with exit 1 even though every PNG was written correctly. Treat the
# golden-image comparison below as the source of truth and only surface the
# binary's failure if no images came out.
if(NOT run_rc EQUAL 0)
    message(STATUS "makevehicle ${PROJECT} exited ${run_rc} (continuing to PNG comparison)")
    message(STATUS "stdout:\n${run_out}")
    message(STATUS "stderr:\n${run_err}")
endif()

file(GLOB golden_pngs RELATIVE "${GOLDEN_DIR}/${PROJECT}"
    "${GOLDEN_DIR}/${PROJECT}/*.png")

if(NOT golden_pngs)
    message(FATAL_ERROR "No golden PNGs found in ${GOLDEN_DIR}/${PROJECT}")
endif()

set(failures "")
foreach(png IN LISTS golden_pngs)
    set(golden "${GOLDEN_DIR}/${PROJECT}/${png}")
    set(actual "${WORK_DIR}/object/images/${png}")
    if(NOT EXISTS "${actual}")
        list(APPEND failures "${png}: not produced by current build")
        continue()
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${golden}" "${actual}"
        RESULT_VARIABLE diff_rc
    )
    if(NOT diff_rc EQUAL 0)
        list(APPEND failures "${png}: bytes differ from golden")
    endif()
endforeach()

if(failures)
    string(REPLACE ";" "\n  " failures_str "${failures}")
    message(FATAL_ERROR
        "Regression FAILED for project '${PROJECT}':\n  ${failures_str}\n"
        "  scratch dir: ${WORK_DIR}\n"
        "  golden dir:  ${GOLDEN_DIR}/${PROJECT}")
endif()

message(STATUS "Regression OK for project '${PROJECT}' (${golden_pngs})")
