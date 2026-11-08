# Add a `.dockerignore` and collapse the 22 source `COPY` lines into one

Done 2026-11-08 (William Emerison Six <billsix@gmail.com> asked for it), on branch `add-dockerignore`.

## What changed

- The `Dockerfile` listed the source tree in 22 `COPY` lines (`.clang-tidy` … `tools`, one layer each)
  plus a late `COPY .clang-format`. They are now one `COPY . /gltron/`, before the cmake build.
- The new `.dockerignore` excludes everything outside the build, so an edit there does not re-run the
  cmake build: `.git`, `.gitattributes`, `.gitignore`, `.dockerignore`, `.claude`, `.crush`,
  `CLAUDE.md`, `Dockerfile`, `Makefile`, `entrypoint`, `tasks`, `bld`, `bld-appimage`, `out`, `*.tar`.
- `entrypoint/` is excluded, but the three files the `Dockerfile`'s earlier `COPY`s name are re-included
  (`!entrypoint/01-install-base.sh`, `!entrypoint/02-install-graphics.sh`,
  `!entrypoint/dotfiles/.emacs.d/install-melpa-packages.el`). The first version excluded them and podman
  failed the build: `no items matching glob … (1 filtered out using …/.dockerignore)`. They now also
  appear under `/gltron/entrypoint/`. That costs no cache: a change to them already re-runs every later
  layer.
- `.clang-format` now arrives with the tree, so editing it re-runs the build (it used to be copied
  after it). It rarely changes.

## Verification (2026-11-08, nested podman, default flags incl. `RUN_SANITIZERS=1`)

- `make image` succeeded: ctest `100% tests passed, 0 tests failed out of 1`, `sanitizer-gate: ALL PASS`.
- File list of `/gltron` in the image, before (old `Dockerfile`) vs after: 371 vs 374 files. The diff is
  exactly the three re-included `entrypoint/` files.
- `podman history gltron`: 35 lines before, 13 after.
