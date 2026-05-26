#!/usr/bin/env bash
#
# Regression tests: re-render every example project with the current
# makevehicle binary and verify each output PNG is byte-identical to the
# golden image checked into tests/regression/golden/.
#
# Goldens were captured from a Release build. Pixel-exact comparison is
# build-config-dependent (compiler emits different float instructions at
# -O0 vs -O2), so only run this against a Release build.
#
# Usage:
#   tests/regression/run.sh [path/to/makevehicle]
#
# If no binary path is given, defaults to build/Release/makevehicle relative
# to the repo root.

set -u

here="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$here/../.." && pwd)"

binary="${1:-$repo_root/build/Release/makevehicle}"
if [ ! -x "$binary" ]; then
    echo "error: makevehicle binary not found or not executable: $binary" >&2
    echo "       pass an explicit path: $0 <path/to/makevehicle>" >&2
    exit 2
fi
# The per-project loop cd's into a scratch dir, so a relative binary path
# would no longer resolve. Make it absolute now.
binary="$(cd "$(dirname "$binary")" && pwd)/$(basename "$binary")"

golden_dir="$here/golden"
work_root="$here/work"
projects=(alpine hybrid single_rail)

overall_rc=0

for project in "${projects[@]}"; do
    work_dir="$work_root/$project"
    rm -rf "$work_dir"
    mkdir -p "$work_dir/object/images"

    # makevehicle resolves mesh/texture paths and the JSON's "preview" field
    # relative to its working directory, so symlink examples/ and masks/ into
    # the scratch dir instead of copying ~MB of assets per invocation.
    ln -s "$repo_root/examples" "$work_dir/examples"
    ln -s "$repo_root/masks"    "$work_dir/masks"

    input_json="examples/$project/$project.json"

    # A nonzero exit from makevehicle does not necessarily mean rendering
    # failed (e.g. a missing parkobj output_directory still leaves the PNGs
    # on disk), so the PNG comparison below is the source of truth.
    if ! (cd "$work_dir" && "$binary" "$input_json"); then
        echo "note: makevehicle $project exited nonzero (continuing to PNG comparison)"
    fi

    shopt -s nullglob
    goldens=("$golden_dir/$project"/*.png)
    shopt -u nullglob
    if [ ${#goldens[@]} -eq 0 ]; then
        echo "FAIL $project: no golden PNGs found in $golden_dir/$project" >&2
        overall_rc=1
        continue
    fi

    failures=()
    for golden in "${goldens[@]}"; do
        png="$(basename "$golden")"
        actual="$work_dir/object/images/$png"
        if [ ! -f "$actual" ]; then
            failures+=("$png: not produced by current build")
            continue
        fi
        if ! cmp -s "$golden" "$actual"; then
            failures+=("$png: bytes differ from golden")
        fi
    done

    if [ ${#failures[@]} -ne 0 ]; then
        echo "FAIL $project:" >&2
        for f in "${failures[@]}"; do
            echo "  $f" >&2
        done
        echo "  scratch dir: $work_dir" >&2
        echo "  golden dir:  $golden_dir/$project" >&2
        overall_rc=1
    else
        echo "OK   $project (${#goldens[@]} images)"
    fi
done

exit $overall_rc
