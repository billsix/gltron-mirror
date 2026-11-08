# Add a `.dockerignore` and collapse the 22 source `COPY` lines into one

**Status:** proposed — needs go-ahead. Created 2026-11-08 (William Emerison Six <billsix@gmail.com>).
**Priority:** 6
**Difficulty:** 2

## BLUF

The `Dockerfile` lists the source tree one entry at a time (`COPY CMake …`, `COPY art …`, … `COPY tools
…`, 22 lines, each its own layer), because without a `.dockerignore` a `COPY . /gltron` would also bring
`.git` (20 MB), `.crush/`, `tasks/`, image tarballs, and host build trees. Add a `.dockerignore` and
replace the list with one `COPY . /gltron/`: one layer instead of 22, and a new top-level source
directory can't be forgotten. Done = `make image` succeeds (its in-image `ctest` passes) and the image's
`/gltron` contains the source the 22 lines copied, minus the excluded files.

## Context

- Dockerfile contract, `.dockerignore` bullet: the personal overlay (github.com/billsix/dotfiles
  `.ai-coding-conventions.personal.md`).
- The `COPY`s are at `Dockerfile` lines ~40–61 and 77 (`.clang-format`). The cmake build + `ctest` run
  right after them, so **anything not excluded that changes will rebuild the image's C build**. That
  includes task docs, entrypoint scripts and `entrypoint/dotfiles`, so the ignore file must exclude
  every non-source path (`tasks/`, `entrypoint/`, `*.md` other than any the build installs,
  `.crush/`, `.git`, `*.tar`, build dirs). That's the trade-off against today's explicit list.
- Separate issue, not this task: the `Dockerfile` still runs the MELPA install at build (line ~37);
  see the conformance snapshot in github.com/billsix/runClaudeInContainer
  `tasks/reference/emacs-container-contract.md`.

## Plan

1. Before editing, snapshot what the image has: `podman run --rm --entrypoint /bin/bash gltron -c 'cd /gltron && find . -type f | sort' > before.txt`.
2. Add `.dockerignore` (exclusions above). Replace lines ~40–61 and the `.clang-format` `COPY` with one
   `COPY . /gltron/` at the same position.
3. `make image`; produce `after.txt` the same way. `diff before.txt after.txt` must show only files the
   ignore file deliberately drops, or none. Record the diff here.
4. If the extra cache invalidation proves annoying (a task-doc edit rebuilding the C build), keep the
   explicit list and use the `.dockerignore` only for hygiene. Record which choice you made.
