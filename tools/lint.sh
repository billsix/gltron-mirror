#!/usr/bin/env bash
#
# lint.sh -- ADVISORY static analysis for the first-party tree (clang-tidy + Cppcheck).
#
# This is NOT a fatal gate. The enforced tier-(a) gate is the compiler warning level
# (-Wall -Wextra ... -Werror, on by default, clang + gcc) wired in CMakeLists.txt. A
# clang-tidy run with the project .clang-tidy produces thousands of readability-* style
# findings on this deliberately-old teaching codebase (e.g. ~95 on one file: uppercase
# literal suffixes, braces-around-statements, math parentheses) -- making those fatal
# would force exactly the churn gltron's "no drive-by modernization" rule forbids. So
# lint.sh runs the BUG-FINDING subset only (bugprone/clang-analyzer/cert), prints what it
# finds, and always exits 0. Triaging its findings into real fixes vs opt-outs, and
# promoting any subset to a fatal gate, is a separate decision (see
# tasks/reference or a follow-up task), not this sweep's job.
#
# Portable: runnable in-container and on the host from the repo root. No options.
set -uo pipefail

# Locate the repo root whether run by path or from elsewhere.
ROOT=$(git rev-parse --show-toplevel 2>/dev/null)
[ -z "$ROOT" ] && ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

BLD="${GLTRON_LINT_BLD:-${TMPDIR:-/tmp}/gltron-lint-bld}"
CC=clang CXX=clang++ cmake -S . -B "$BLD" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DGLTRON_WARN=OFF >/dev/null 2>&1

FILES=$(find src nebu \( -name '*.c' -o -name '*.cpp' \) ! -name '*-macos.*' ! -name '*-win32.*')

echo "== clang-tidy (bug-finding checks; advisory) =="
if command -v run-clang-tidy >/dev/null 2>&1; then
  run-clang-tidy -p "$BLD" -quiet \
    -checks='-*,bugprone-*,clang-analyzer-*,cert-*,-bugprone-easily-swappable-parameters' \
    -header-filter="$ROOT/(src|nebu)/" \
    "$ROOT/(src|nebu)/.*\.(c|cpp)\$" 2>/dev/null || true
else
  for f in $FILES; do
    clang-tidy -p "$BLD" -quiet \
      -checks='-*,bugprone-*,clang-analyzer-*,cert-*,-bugprone-easily-swappable-parameters' \
      -header-filter="$ROOT/(src|nebu)/" "$f" 2>/dev/null || true
  done
fi

echo "== cppcheck (advisory) =="
if command -v cppcheck >/dev/null 2>&1; then
  cppcheck --enable=warning,style,performance,portability --quiet \
    --suppress=missingIncludeSystem -I src/include -I nebu/include $FILES 2>&1 || true
else
  echo "cppcheck not installed; skipping"
fi

echo "== lint.sh is advisory; exit 0 regardless of findings =="
exit 0
