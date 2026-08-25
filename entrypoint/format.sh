#!/bin/env bash

# The script exits nonzero if clang-format failed on ANY file -- otherwise
# the exit code is the last loop iteration's alone, masking earlier failures
status=0
for x in $(find nebu src/ \( -iname "*.c" -o -iname "*.cpp" -o -iname "*.h" -o -iname "*.hpp" \)) ; do
              clang-format -i $x || status=1 ;
              done;
exit $status
