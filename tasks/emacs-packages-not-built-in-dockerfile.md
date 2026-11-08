# Emacs packages are not installed or compiled in the Dockerfile

**Status:** in progress
**Priority:** 3
**Difficulty:** 2
Created 2026-11-08 (William Emerison Six <billsix@gmail.com> asked for it).
**Part of:** github.com/billsix/runClaudeInContainer `tasks/emacs-contract-conformance.md` (item 1, the
build-time MELPA install, and item 5, the run-time compile).

## BLUF

`make image` runs `emacs --batch --load install-melpa-packages.el` under `USE_EMACS=1`: it downloads and
byte-compiles the whole MELPA set at image build, and `make shell` then mounts the vendored
`entrypoint/dotfiles/.emacs.d/` over `/root/.emacs.d`, so the result is never used. Done = the image
installs only the Emacs binary (from an `entrypoint/03-install-emacs.sh` group script), `init.el`
byte-compiles the vendored packages on Emacs's first start, `make image` builds with no MELPA fetch, and
Emacs starts cleanly against the mount.

## Context

- The Emacs contract: the `emacs-container-contract.md` reference doc in github.com/billsix/runClaudeInContainer
  `tasks/reference/`. The worked C example is github.com/billsix/Craft
  `tasks/archive/2026/11/08/emacs-packages-not-built-in-dockerfile.md` (commit `f6f7d2ba`).
- Today: `Dockerfile` lines 28–38 install `emacs` and run the MELPA install; `emacs` is also in
  `entrypoint/01-install-base.sh`, so `USE_EMACS=0` still installs it; the Makefile's `ELPA_MOUNT` mounts
  the whole `.emacs.d/`; `init.el` has no run-time compile; `.gitignore` already ignores `*.elc`/`*.eln`.

## Decision: keep the whole-`.emacs.d` mount (deviation from Craft)

Craft mounts only `elpa/` and bakes `init.el`/`helm.el`/`preferences.el` into the image. gltron's
`Dockerfile` copies the source with a single `COPY . /gltron/` right before the cmake build + ctest, so
baking the config would either re-run the game build on every config edit (if `.dockerignore`
re-included it) or need a second dotfiles `COPY`. Keeping the existing whole-`.emacs.d` mount needs no
image change at all for config, and config edits stay live without `make image`. This is recorded as a
gltron variant of the contract's "mount only `elpa/`" rule.

## Plan

1. New `entrypoint/03-install-emacs.sh` (`emacs emacs-gtk+x11 emacs-pgtk`, as Craft); drop `emacs` from
   `01-install-base.sh`; the `Dockerfile` dispatches it under `USE_EMACS` in the install `RUN`, and the
   MELPA `COPY` + `RUN` go away. `.dockerignore` re-includes the new script instead of
   `install-melpa-packages.el`.
2. `init.el`: `(byte-recompile-directory package-user-dir 0)` after `(package-initialize)`.
3. Makefile: refresh the `ELPA_MOUNT` / `update-emacs-packages` comments (the target itself still works:
   it needs only the Emacs binary in the image).
4. `CLAUDE.md`: describe the new behavior.

## Verification

- `make image` (default flags): no MELPA fetch in the log; `/usr/bin/emacs` present; no
  `/root/.emacs.d/elpa` in the image.
- `make shell-exec CMD='emacs --batch -l /root/.emacs.d/init.el …'`: exits 0, compiles `.elc` into the
  host `elpa/`, `git status` clean.
- `make image USE_EMACS=0`: no `emacs` on `PATH`.
- `tools/check_helm_file_at_point.sh` (runClaudeInContainer) on the config: 7/7.

## Open questions

None.
