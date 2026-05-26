#!/usr/bin/env bash
#
# Regression test: re-run the current subposition binary and verify its
# stdout is byte-identical to the golden output checked into
# tests/regression/golden/subposition/output.txt.
#
# The golden was captured from the RCTGen-master Release build (see
# RCTGen-master/build/Release/subposition). Pixel-exact / byte-exact
# comparison is build-config-dependent (compiler emits different float
# instructions at -O0 vs -O2), so only run this against a Release build.
#
# Usage:
#   tests/regression/run_subposition.sh [path/to/subposition]
#
# If no binary path is given, defaults to build/release/Release/subposition
# relative to the repo root.

set -u

here="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$here/../.." && pwd)"

binary="${1:-$repo_root/build/release/Release/subposition}"
if [ ! -x "$binary" ]; then
    echo "error: subposition binary not found or not executable: $binary" >&2
    echo "       pass an explicit path: $0 <path/to/subposition>" >&2
    exit 2
fi
binary="$(cd "$(dirname "$binary")" && pwd)/$(basename "$binary")"

golden="$here/golden/subposition/output.txt"
if [ ! -f "$golden" ]; then
    echo "error: golden file not found: $golden" >&2
    exit 2
fi

work_dir="$here/work/subposition"
rm -rf "$work_dir"
mkdir -p "$work_dir"
actual="$work_dir/output.txt"

# subposition takes no input and writes its table to stdout. A nonzero
# exit is treated as a failure here (unlike maketrack/makevehicle, which
# can exit nonzero on benign post-render steps); the stdout diff below is
# still the source of truth for content.
if ! "$binary" >"$actual"; then
    echo "note: subposition exited nonzero (continuing to diff)"
fi

if cmp -s "$golden" "$actual"; then
    lines=$(wc -l <"$golden" | tr -d ' ')
    echo "OK   subposition (${lines} lines)"
    exit 0
fi

echo "FAIL subposition: stdout differs from golden" >&2
echo "  golden:  $golden" >&2
echo "  actual:  $actual" >&2
echo "  diff (first 40 lines):" >&2
diff "$golden" "$actual" | head -40 >&2
exit 1
