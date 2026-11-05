# Upgrade the container base from Fedora 44 to Fedora 45

**Status:** in progress
**Priority:** 3
**Difficulty:** 4

## BLUF
Bump `Dockerfile` to `fedora:45`; `make image` green nested (it builds the game
with meson + runs ctest at image-build time). "Done" = image builds on 45.

## Context
Fleet-wide Fedora 44 → 45 bump (William Emerison Six <billsix@gmail.com>,
2026-11-05). Fedora 45: GCC 16.2 / clang LLVM 23. gltron is C (SDL/OpenGL);
risk is new GCC-16 diagnostics in the modernized C under its warning posture.

## Plan
1. `fedora:44` → `fedora:45`. 2. `make image` nested. 3. Fix toolchain breakage
minimally (discretion; log here). 4. Stage by path; archive.

## Notes / findings
(appended as the build runs)
