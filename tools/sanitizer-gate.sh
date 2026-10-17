#!/usr/bin/env bash
#
# sanitizer-gate.sh -- build gltron under UBSan(trap) and ASan, exercise it
# headless, and fail on any undefined behavior or memory error.
#
# Invoked by the Dockerfile when RUN_SANITIZERS=1 (see Makefile.docker). Also
# runnable by hand inside the image:  bash tools/sanitizer-gate.sh [SRC_DIR]
# SRC_DIR defaults to the repo root (the dir two levels up from this script).
#
# Design notes (see tasks/add-sanitizer-gate.md):
#  - Needs a software GL context: Xvfb + Mesa llvmpipe. So it requires the image's
#    graphics group (USE_GRAPHICS=1, which installs xorg-x11-server-Xvfb + mesa).
#  - The run is bounded by `timeout`; no in-game frame-limit source hook is used
#    (teaching-artifact rule: no drive-by source edits). A clean timeout is a pass.
#  - timedemo mode is enabled by editing the THROWAWAY install tree's main.lua,
#    never the committed source, so the run exercises AI/physics/camera/render.
#  - ASan's by-design exit-time leaks are ignored (detect_leaks=0); the gate targets
#    corruption, not leaks. Add tools/asan-gate.supp to silence library noise.
#  - Multi-step: every config runs even if an earlier one failed, but ANY failure
#    fails the gate (status accumulation + `exit $status`), per the shared
#    shell-and-gate-scripts convention.

set -uo pipefail

SRC_DIR="${1:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"}"
RUN_SECONDS="${GLTRON_GATE_SECONDS:-25}"
SUPP="${SRC_DIR}/tools/asan-gate.supp"

if ! command -v xvfb-run >/dev/null 2>&1; then
  echo "sanitizer-gate: xvfb-run not found -- RUN_SANITIZERS=1 requires the graphics" >&2
  echo "                group (build the image with USE_GRAPHICS=1)." >&2
  exit 1
fi

status=0

# Build gltron with the given sanitizer flags, install to a private prefix with its
# own data tree, flip the installed main.lua into timedemo mode, then run it headless
# under a software GL context for RUN_SECONDS. Prints a verdict; sets `status=1` on
# any build/run failure. Args: <label> <extra-run-env> <cflags...>
run_config() {
  local label="$1"; shift
  local run_env="$1"; shift
  local sanflags="$*"

  local bld="/tmp/san-${label}"
  local inst="/tmp/san-${label}-inst"
  rm -rf "${bld}" "${inst}"

  # configure + build + install
  {
    echo "=== [${label}] configure + build + install ==="
    CC=clang CXX=clang++ cmake -S "${SRC_DIR}" -B "${bld}" -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_INSTALL_PREFIX="${inst}" \
      -DGLTRON_DATA_DIR="${inst}/share/gltron" \
      -DCMAKE_C_FLAGS="${sanflags}" \
      -DCMAKE_CXX_FLAGS="${sanflags}" \
      -DCMAKE_EXE_LINKER_FLAGS="${sanflags}" \
    && cmake --build "${bld}" -j \
    && cmake --install "${bld}"
  } || { echo "sanitizer-gate: [${label}] build/install FAILED"; status=1; return; }

  # Enable timedemo in the throwaway install tree only (never the source).
  local mainlua="${inst}/share/gltron/scripts/main.lua"
  if [ -f "${mainlua}" ]; then
    sed -i 's/^timedemo = nil/timedemo = 1/' "${mainlua}"
  fi

  # Bounded headless run under software GL. Capture the REAL inner exit status
  # (xvfb-run's own status is unreliable across versions).
  echo "=== [${label}] headless run (${RUN_SECONDS}s cap) ==="
  local rcfile="/tmp/san-${label}.rc"
  env ${run_env} LIBGL_ALWAYS_SOFTWARE=1 SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy \
    xvfb-run -a -s "-screen 0 1024x768x24" \
      bash -c "timeout -s TERM ${RUN_SECONDS} '${inst}/bin/gltron'; echo \$? > '${rcfile}'" \
      >"/tmp/san-${label}.log" 2>&1
  local rc; rc="$(cat "${rcfile}" 2>/dev/null || echo 255)"
  tail -15 "/tmp/san-${label}.log" || true

  # Pass: clean exit (0) or killed by the timeout (124 from timeout, 143 if SIGTERM
  # propagated). Fail: any other signal (SIGILL=132 UBSan trap, SIGABRT=134 ASan) or
  # a nonzero startup error.
  case "${rc}" in
    0|124|143) echo "sanitizer-gate: [${label}] PASS (exit ${rc})" ;;
    *)         echo "sanitizer-gate: [${label}] FAIL (exit ${rc}) -- see /tmp/san-${label}.log"; status=1 ;;
  esac

  rm -rf "${bld}" "${inst}"
}

asan_env="ASAN_OPTIONS=detect_leaks=0:abort_on_error=1"
[ -f "${SUPP}" ] && asan_env="${asan_env}:suppressions=${SUPP}"

run_config "ubsan" "" "-fsanitize=undefined -fsanitize-trap=undefined"
run_config "asan"  "${asan_env}" "-fsanitize=address -fno-omit-frame-pointer"

if [ "${status}" -eq 0 ]; then
  echo "sanitizer-gate: ALL PASS"
else
  echo "sanitizer-gate: FAILURES above"
fi
exit "${status}"
