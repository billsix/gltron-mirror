# Standardize gltron's container build + add Emacs C support

**Status:** in progress — **Phase 1 done 2026-10-17** (`Makefile.docker`→`Makefile` on the standard
contract: `help` default, `PODMAN_RUN_FLAGS`, `shell-exec`, `image-export`/`import`, and a working
portable **`make format`** — which also fixed a latent `format.sh` no-op where `find nebu src` ran
from the wrong cwd in-container; docs repointed). **Phase 2 remaining:** the Emacs clangd/eglot
vendoring + `make update-emacs-packages` + `clang-tools-extra`/`emacs` in the Dockerfile.
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

### Phase 2 — Emacs C support (clangd + eglot) + update-emacs-packages
- [ ] Vendor `entrypoint/dotfiles/.emacs.d/` from the exemplar (`init.el`, `preferences.el`,
      `helm.el`, `install-melpa-packages.el`), adjusted for gltron (clangd/eglot for C; the
      `compile_commands.json` gltron's CMake already emits is the clangd input).
- [ ] Add `ARG USE_EMACS=0` to the `Dockerfile` (Makefile passes `1` by default per fleet
      convention), installing `emacs` + running the melpa install script when on; ensure
      `clang-tools-extra` (clangd) is in the dnf list (`entrypoint/01-install-base.sh`).
- [ ] Add the `ELPA_MOUNT` + **`update-emacs-packages`** target (copy the exemplar's: force
      `USE_EMACS=1`, rebuild image, wipe+reinstall `elpa/`, strip `*.elc`/`*.eln`, `git add -f` the
      elpa tree). This is the target the billsEmacsConfigs top-level script will call.

### Phase 3 — verify
- [ ] Nested: `make image`, `make shell` (emacs opens a C file, clangd resolves a symbol / jumps to
      def), `make shell-exec CMD='...'`, `make update-emacs-packages` (elpa refreshed + staged),
      `make image-export`. The game still builds and the sanitizer gate (`RUN_SANITIZERS=1`) passes.

## Notes / caveats
- **Do not switch gltron to meson** — only the Makefile/Dockerfile *contract* and the emacs layer are
  in scope here; gltron stays on CMake (the `compile_commands.json` it emits is what clangd needs).
- gltron is a GUI game, so its Makefile legitimately differs from the headless exemplar (display
  passthrough); that variation is deliberate — conform the *structure*, not those mounts away.
- Related: `/foo/opt/billsEmacsConfigs` gets a top-level script that runs `make image &&
  make update-emacs-packages` per language dir; gltron's new target should behave the same way so a
  fleet-wide refresh could include it if ever desired.
