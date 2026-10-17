#!/usr/bin/env bash
#
# discover.sh -- regenerate the Phase-1 warning worklist for the C modernization
# sweep (tasks/c-modernization-sweep.md). Configures a throwaway build of the
# first-party tree under the C23 pin + the sweep's raised warning set as plain
# warnings (NO -Werror, ninja -k 0 keep-going), and captures every diagnostic.
#
# The warning flags are passed via CMAKE_C_FLAGS / CMAKE_CXX_FLAGS so this does
# NOT require the CMakeLists gate to be wired yet; the flags land on first-party
# TUs only (the vendored/system deps are built elsewhere, not in this tree).
#
# Output snapshot: data/warnings-first-party.txt, data/by-class.txt, data/by-file.txt
# Run from anywhere; resolves the repo root via git.
set -uo pipefail

ROOT=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$ROOT"
BLD=$(mktemp -d)
DATA="$(dirname "$0")/data"
mkdir -p "$DATA"

WFLAGS="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wvla \
-Wstrict-prototypes -Wold-style-definition -Wwrite-strings -Wformat=2"
# -Wstrict-prototypes / -Wold-style-definition are C-only; drop them for C++.
CXXWFLAGS="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wvla \
-Wwrite-strings -Wformat=2"

CC=clang CXX=clang++ cmake -S . -B "$BLD" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="$WFLAGS" -DCMAKE_CXX_FLAGS="$CXXWFLAGS" >/dev/null

LOG="$BLD/build.log"
ninja -C "$BLD" -k 0 >"$LOG" 2>&1 || true   # keep-going: report every warning

# first-party only (src/, nebu/), repo-relative, sorted
grep -E 'warning:' "$LOG" | grep -E "$ROOT/(src|nebu)/" | grep -vE 'third_party|HandmadeMath' \
  | sed -E "s|$ROOT/||" | sort > "$DATA/warnings-first-party.txt"
grep -E 'warning:' "$LOG" | grep -oE '\[-W[a-z0-9-]+\]' | sort | uniq -c | sort -rn > "$DATA/by-class.txt"
grep -E 'warning:' "$LOG" | grep -oE "$ROOT/(src|nebu)/[^:]+" | sed "s|$ROOT/||" \
  | sort | uniq -c | sort -rn > "$DATA/by-file.txt"

echo "first-party warnings: $(wc -l < "$DATA/warnings-first-party.txt")"
rm -rf "$BLD"
