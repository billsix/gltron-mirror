# Upgrade the container base from Fedora 44 to Fedora 45 (done)

Done 2026-11-05 (William Emerison Six <billsix@gmail.com>). Part of a fleet-wide
Fedora 44 -> 45 bump.

## What changed

- `Dockerfile`: `registry.fedoraproject.org/fedora:44` -> `fedora:45`.
- `nebu/video/video_system.c`: marked the `bitdepth` global `[[maybe_unused]]`.

## Toolchain finding (LLVM 23)

The main build + ctest passed, but the **sanitizer gate** (clang `-Werror`)
failed on `nebu/video/video_system.c:16`: `bitdepth` is a static global set in
the SDL display path but never read (vestigial after the SDL2 -> SDL3 port; the
other `bitdepth` names in the tree are unrelated locals). Fedora 45 clang (LLVM
23) adds `-Wunused-but-set-global`, which `-Werror` made fatal. Marked it with
the C23 `[[maybe_unused]]` attribute (the build is `CMAKE_C_STANDARD 23`) --
same fix class as spimulator. Only breakage from the toolchain bump; `make image`
(build + ctest + sanitizer gate) green on 45. Image `localhost/gltron:latest`.
