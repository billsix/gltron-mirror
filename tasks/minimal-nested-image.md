# Lean image for nested-podman builds — what "minimal" means for gltron

**Status:** proposed — research done 2026-09-10 (survey of the Dockerfile + Makefile from the
runClaudeInContainer sandbox); **implementation needs go-ahead**. One of the per-project children of
runClaudeInContainer `tasks/minimal-image-for-nested-podman-standard.md` (the convention: every optional-feature build flag defaults to its lean value when
`NESTED_PODMAN=1`); the fleet-wide findings table is runClaudeInContainer `tasks/reference/minimal-nested-images.md`. Created 2026-09-10 at the maintainer's
request (William Emerison Six <billsix@gmail.com>: "go through all of my projects with CLAUDE.md …
research what a minimal nested podman container would be for them").
**Priority:** 5
**Difficulty:** 3

## BLUF

Make `make image` inside a sandbox (which exports `NESTED_PODMAN=1`) build a lean image that fits
the nested RAM store and still runs this project's gate, while a host `make image` stays
byte-identical — via the idiom `FLAG ?= $(if $(filter 1,$(NESTED_PODMAN)),0,1)` on each optional-feature flag (the `PODMAN_RUN_FLAGS`
pattern applied to build flags; reference implementation: runCrushInContainer `client/Makefile`,
`FULL_TOOLCHAIN`). Done = the flags below carry the nested-aware default, a nested `make image`
builds and passes the gate, both image sizes are measured and recorded here and in `CLAUDE.md`.

## Context — read first

- runClaudeInContainer `tasks/reference/minimal-nested-images.md` — the standard, the idiom, the rules (a project's *gates* and *product build deps* are never
  trimmed; only editors, docs toolchains, notebooks, GUI extras), and every project's row.
- This repo's `Dockerfile`, `Makefile` (flag block + `image` target), `entrypoint/*install*.sh`.
- The flag-coverage rule (cross-project `CLAUDE.md` › "Verification gates in nested containers"): a
  lean build verifies nothing about the layers it skips — when a change touches what a skipped layer
  consumes, build with that flag ON.

## Findings (2026-09-10)

**What the image installs today.** Flag `USE_GRAPHICS` (1) gates `02-install-graphics.sh` (mesa-dri-drivers + the X11 client libs, 18 pkgs). `01-install-base.sh` (34 pkgs) is unconditional and includes **emacs** and **ffmpeg** alongside the real build deps (SDL3-devel, glew, cmake, clang, gcc…). The image also builds and `ctest`s gltron at image time — that is the gate and stays. Build file is `Makefile.docker`.

**What "minimal" is here.** `USE_GRAPHICS` → 0 when nested (the nested store never has a display; headless GL verification uses the *sandbox's* Xvfb/Mesa, not the image's X libs — check that `ctest` needs no X first). A new `USE_EMACS` gating `emacs` (and `ffmpeg`, if it is only for capture).

**Notes.** If `ctest` turns out to need an X server even headlessly, `USE_GRAPHICS` must stay 1 nested — say so in the task rather than weakening the gate.

## Plan

- [ ] Confirm what `ffmpeg` is for (grep the tree; if nothing at build/test time, gate it with emacs under `USE_DEV_EXTRAS`/`USE_EMACS`).
- [ ] Split `emacs` (+ `ffmpeg`) out of `01-install-base.sh` into a flag-gated script; Dockerfile ARG + `if`.
- [ ] `Makefile.docker`: `USE_GRAPHICS` and the new flag get the nested-aware default.
- [ ] Verify: nested `make -f Makefile.docker image` still passes the image-time `ctest`; measure both sizes; `CLAUDE.md`.
- [ ] Record both sizes (host full vs nested lean) here and in `CLAUDE.md`; add the standard's one-line
      rule to `CLAUDE.md` ("nested = lean image automatically; `FLAG=1` overrides").

## Open questions

None — the standard's decisions (dnf-only, gates never trimmed) were the maintainer's on 2026-09-10;
anything project-specific to decide is flagged inline above.
