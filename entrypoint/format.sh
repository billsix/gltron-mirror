#!/bin/env bash

# The script exits nonzero if clang-format failed on ANY file -- otherwise
# the exit code is the last loop iteration's alone, masking earlier failures
# (the flaw found in gacalc's format gate, 2026-07-29).  (The stale lib3ds/
# path is already tracked in tasks/container-build-cleanup.md.)
status=0
for x in $(find lib3ds/ nebutest/ nebu src/ \( -iname "*.c" -o -iname "*.cpp" -o -iname "*.h" -o -iname "*.hpp" \)) ; do
              clang-format -i $x || status=1 ;
              done;
exit $status
