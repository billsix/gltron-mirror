#!/usr/bin/env bash
#
# Format all first-party C/C++ with clang-format. Portable: runnable in-container
# (the source is bind-mounted at /gltron) and on the host from anywhere.
#
# cd to the repo root so the relative paths below resolve in either environment:
# in the container /gltron/nebu exists; on the host fall back to the git toplevel.
if [ -d /gltron/nebu ]; then
    cd /gltron
else
    root=$(git rev-parse --show-toplevel 2>/dev/null) && [ -n "$root" ] && cd "$root"
fi

# Exit nonzero if clang-format failed on ANY file -- a plain loop would exit with
# only the last iteration's status, masking earlier failures.
status=0
for x in $(find nebu src \( -iname '*.c' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.hpp' \)); do
    clang-format -i "$x" || status=1
done
exit $status
