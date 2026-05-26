#!/usr/bin/env bash
#
# Runs every regression script in tests/regression and verifies each one
# emitted the expected number of "OK" lines. A passing maketrack /
# makevehicle run prints one "OK" per example project (alpine, hybrid,
# single_rail); subposition prints a single "OK".
#
# Usage:
#   tests/regression/run_all.sh [path/to/build-bindir]
#
# Defaults the binary directory to build/release/Release relative to the
# repo root (matches the layout produced by `cmake --preset release` +
# `cmake --build --preset release`). Each sub-script is invoked with an
# explicit binary path so this driver doesn't depend on the sub-script
# defaults.

set -u

here="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$here/../.." && pwd)"

bindir="${1:-$repo_root/build/release/Release}"
if [ ! -d "$bindir" ]; then
    echo "error: binary directory not found: $bindir" >&2
    echo "       pass an explicit path: $0 <path/to/build-bindir>" >&2
    exit 2
fi

# script-name : binary-name : expected OK lines on success
tests=(
    "run_maketrack.sh:maketrack:3"
    "run_makevehicle.sh:makevehicle:3"
    "run_subposition.sh:subposition:1"
)

overall_rc=0

for spec in "${tests[@]}"; do
    IFS=: read -r script binary expected <<<"$spec"

    full_script="$here/$script"
    full_binary="$bindir/$binary"

    if [ ! -x "$full_script" ]; then
        echo "FAIL $script: not found or not executable at $full_script" >&2
        overall_rc=1
        continue
    fi
    if [ ! -x "$full_binary" ]; then
        echo "FAIL $script: binary not found at $full_binary" >&2
        overall_rc=1
        continue
    fi

    log="$(mktemp)"
    "$full_script" "$full_binary" >"$log" 2>&1
    rc=$?

    # grep -c exits nonzero on zero matches; capture the count regardless.
    ok_count=$(grep -c '^OK' "$log")

    if [ "$rc" -eq 0 ] && [ "$ok_count" -eq "$expected" ]; then
        echo "PASS $script ($ok_count/$expected OK)"
    else
        echo "FAIL $script: rc=$rc, expected $expected OK line(s), got $ok_count" >&2
        sed 's/^/  | /' "$log" >&2
        overall_rc=1
    fi

    rm -f "$log"
done

exit $overall_rc
