# Standardize gltron's container build + add Emacs C support

**Status:** DONE 2026-10-17 — all phases complete and build-verified. One hands-on check remains for
the maintainer: open a C file in an interactive `make shell` emacs and confirm clangd resolves a
symbol (needs a GUI session; everything it depends on — `clang-tools-extra`, the vendored `.emacs.d`
with `--compile-commands-dir=/gltron/bld`, a prior `make build` — is in place).
**Priority:** 7
**Difficulty:** 4
**Created:** 2026-10-17 (William Emerison Six <billsix@gmail.com>)

## BLUF

Bring gltron's container build onto the fleet's standard **Dockerfile + Makefile contract** and add
**Emacs C support with `make update-emacs-packages`**, modeled on `/foo/opt/billsEmacsConfigs/c`
(the exemplar: `meson.build` aside, its `Dockerfile`/`Makefile`/`entrypoint/` are the contract).
Concretely: rename `Makefile.docker` → `Makefile`, add the missing standard targets/variables, and
vendor a C-focused `.emacs.d` (clangd + eglot) refreshed by a `update-emacs-packages` target —
**keeping gltron's deliberate specifics** (X11/Wayland display passthrough, `USE_GRAPHICS`, the
sanitizer gate, and **CMake** — do NOT switch gltron to meson; only the Makefile/Dockerfile
*contract* and the emacs layer change). "Done" = `make image` / `make shell` / `make shell-exec` /
`make format` / `make update-emacs-packages` / `make image-export` all work (nested and on host),
clangd resolves symbols in an interactive `make shell` emacs, and the game still builds + the
sanitizer gate still passes.

## Context (read first)

1. **Exemplar — `/foo/opt/billsEmacsConfigs/c/`** ("that directory has everything you need to know"):
   - `Makefile`: `.DEFAULT_GOAL := help`; `CONTAINER_CMD`; `PODMAN_RUN_FLAGS ?= $(if $(filter 1,$(NESTED_PODMAN)),--cgroups=disabled)`; a single shared `SHELL_RUN_FLAGS` feeding both `shell` and `shell-exec` (so they can't drift); `USE_EMACS` flag gating an `ELPA_MOUNT` of the vendored `.emacs.d`; targets `image` / `shell` / `shell-exec SCRIPT=|CMD=` / `format` / `update-emacs-packages` / `image-export` / `image-import` / `help`.
   - `Dockerfile`: `ARG USE_EMACS=0`; installs `clang clang-tools-extra gcc gdb lldb meson ninja …`; when `USE_EMACS=1`, installs `emacs` and runs `emacs --batch --load .emacs.d/install-melpa-packages.el`; a bash `exit()` hook that runs `format.sh`/`lint.sh` on shell exit.
   - `entrypoint/`: `shell.sh`, `format.sh`, `lint.sh`, `buildDebug.sh`, `entrypoint.sh`, and `dotfiles/.emacs.d/{init.el,preferences.el,helm.el,install-melpa-packages.el}`. The C LSP is **clangd** (from `clang-tools-extra`) + **eglot** in `init.el`, pointed at the `compile_commands.json` the build emits.
2. **gltron today — `Makefile.docker`** (only Makefile): targets `image` / `build` / `shell` / `appimage` / `format` / `help`; `USE_GRAPHICS`/`RUN_SANITIZERS` build-args; X11 (`USE_X`) + Wayland passthrough for the on-screen game; mounts the repo at `/gltron`. `Dockerfile` is Fedora 44, per-group `entrypoint/0N-install-*.sh`, builds gltron + the sanitizer gate at image-build time. CMake (not meson) — keep it.
3. **Conventions** — the container-per-project template and the nested-podman `PODMAN_RUN_FLAGS` rule
   live in `github.com/billsix/runClaudeInContainer` (`CLAUDE.md`, `tasks/reference/nested-podman-design.md`,
   `tasks/reference/nested-run-and-gates.md`). gltron's own `CLAUDE.md` documents the current build.

## Plan

### Phase 1 — Makefile.docker → Makefile on the standard contract  — DONE 2026-10-17
- [x] `git mv Makefile.docker Makefile`; repointed `CLAUDE.md`, `README`, `INSTALL` (task-doc
      mentions of `Makefile.docker` are historical and left).
- [x] Adopted the standard structure: `PODMAN_RUN_FLAGS` (nested), shared `SHELL_RUN_FLAGS` feeding
      `shell` + new `shell-exec SCRIPT=|CMD=`, `image-export`/`image-import`, `help` as
      `.DEFAULT_GOAL`. Preserved the X11/Wayland/`USE_X` mounts, `USE_GRAPHICS`, `RUN_SANITIZERS`,
      `build`/`appimage`, and the `/gltron` mount root.
- [x] **`make format`** works and is portable: runs `entrypoint/format.sh` in the gltron image when
      built (reproducible), else on the host with its clang-format; no `-f`, no sanitizer-gate
      rebuild. Fixed `format.sh` to `cd` to the repo root (`/gltron` in-container, git toplevel on
      host) — it previously `find`-ed `nebu src/` from the wrong cwd and silently formatted nothing.
- [ ] (optional) wire the advisory `tools/lint.sh` as a `lint` target, mirroring the exemplar.

### Phase 2 — Emacs C support (clangd + eglot) + update-emacs-packages  — infra DONE 2026-10-17
- [x] Vendored `entrypoint/dotfiles/.emacs.d/` from the exemplar (`init.el`, `preferences.el`,
      `helm.el`, `install-melpa-packages.el`). Adjusted for gltron: `my-lsp-root` → `/gltron/`, and
      `lsp-clients-clangd-args '("--compile-commands-dir=/gltron/bld")'` so clangd finds the
      `compile_commands.json` gltron's CMake writes to `bld/` (run `make build` first).
- [x] `ARG USE_EMACS=0` in the `Dockerfile` (Makefile passes `1`): COPYs `install-melpa-packages.el`
      and, when on, installs `emacs` + runs the MELPA install at image-build. `clang-tools-extra`
      (clangd) **and** `emacs` were already unconditional in `entrypoint/01-install-base.sh`.
- [x] `ELPA_MOUNT` (bind-mounts the vendored `.emacs.d` into `shell`/`shell-exec` when `USE_EMACS=1`)
      + the **`update-emacs-packages`** target (rebuild `USE_EMACS=1 RUN_SANITIZERS=0`, wipe+reinstall
      `elpa/`, strip `*.elc`/`*.eln`, `git add -f`). `.gitignore` added for compiled artifacts + build
      dirs. This is the target the billsEmacsConfigs top-level script invokes.
- [ ] **Verify:** `make image USE_EMACS=1` builds green (automated, in progress); `make
      update-emacs-packages` populates + stages `elpa/`. "clangd resolves a symbol in an interactive
      `make shell` emacs" needs a GUI session — the maintainer's hands-on check.

### Phase 3 — verify  — automated checks DONE 2026-10-17
- [x] `make image USE_EMACS=1` builds green (gltron links; emacs + MELPA install succeed; image
      `47f6417b`). The `RUN_SANITIZERS=1` gate was already verified green earlier this session.
- [x] `make format` runs in-container and formats `nebu`/`src` (and caught one real misalignment).
- [x] `make update-emacs-packages` populated + staged the vendored `elpa/` (full package set incl
      compat/lsp-mode/magit; `*.elc`/`*.eln` stripped; committed `64958953`).
- [ ] **Maintainer hands-on (GUI):** `make shell`, open a C file, confirm clangd resolves/jumps to a
      definition. Also optional spot-checks: `make shell-exec CMD='...'`, `make image-export`.

## Notes / caveats
- **Do not switch gltron to meson** — only the Makefile/Dockerfile *contract* and the emacs layer are
  in scope here; gltron stays on CMake (the `compile_commands.json` it emits is what clangd needs).
- gltron is a GUI game, so its Makefile legitimately differs from the headless exemplar (display
  passthrough); that variation is deliberate — conform the *structure*, not those mounts away.
- Related: `/foo/opt/billsEmacsConfigs` gets a top-level script that runs `make image &&
  make update-emacs-packages` per language dir; gltron's new target should behave the same way so a
  fleet-wide refresh could include it if ever desired.
