# gltron

A 3D lightcycle game from 1999 (Andreas Umbach), written in C against
**fixed-function OpenGL**. The owner of this fork (Bill Six, billsix@gmail.com)
maintains it as an **educational vehicle for explaining graphics concepts** —
the fixed-function pipeline is the point. **Do not replace it with shaders or
introduce a modern renderer abstraction unless explicitly asked.**

## Status

- Migrated from SDL 1.2 → SDL2 → **SDL3** on this fork.
- Runs on **Linux** and **Windows**. macOS builds and runs as of `21113968`,
  with a known hack around SDL2 sound (see commit message). The macOS path
  has not been re-tested on SDL3 yet.
- Audio uses **SDL3_sound**.
- Scripting embedded via **system Lua 5.4** (linked via `find_package(Lua)`).
- Build system: **CMake ≥ 3.21** (Ninja recommended). See `INSTALL` for the
  full dep list and option flags. Out-of-tree only.
- A Fedora-based **podman** dev container is provided (`Makefile`,
  `Dockerfile`); `make image` builds the image and
  compiles gltron at image-build time, `make shell`
  drops into a shell, `make build` re-runs the
  CMake build against the mounted source tree (writes to `./bld/`), and
  `make format` clang-formats the C/C++ (in the image if built, else on the host).
  `make help` lists all targets. (Renamed from `Makefile.docker` onto the fleet
  Makefile/Dockerfile contract 2026-10-17.)
  CMake natively emits `compile_commands.json`. `clang-tidy` and
  `clang-format` are configured (`.clang-tidy`, `.clang-format`).
- **Sanitizer gate** at image-build time: `make image`
  (`RUN_SANITIZERS=1`, the default; `RUN_SANITIZERS=0` skips) also compiles gltron
  under **UBSan-trap** and **ASan** and runs it headless (Xvfb + Mesa llvmpipe, in
  timedemo mode), failing the image on any UB or memory error. Needs
  `USE_GRAPHICS=1`. Script: `tools/sanitizer-gate.sh`; headless-GL recipe + rationale
  in `tasks/archive/2026/10/17/add-sanitizer-gate.md`.

## Layout

- `src/game/` — game logic: engine, AI (`computer.c`, `computer_utilities.c`),
  events, camera, GUI, level, scripting glue.
- `src/video/`, `src/audio/`, `src/input/`, `src/configuration/`,
  `src/filesystem/`, `src/scripting/`, `src/base/` — subsystem code.
- `src/include/` — public headers; game data structs live in
  `src/include/game/game_data.h`.
- `nebu/` — small in-tree engine library (Nebu).
- (vendored deps fully removed; Lua and lib3ds were dropped in favour of system packages / dead-code excision)
- `data/`, `art/`, `levels/`, `music/`, `sounds/` — assets.
- `scripts/` — Lua game scripts.
- `tools/` — dev/build helper scripts (asset/texture/font conversion:
  `crash-tex.pl`, `font-tex.pl`, `artpack-conversion/`). Distinct from
  runtime `scripts/`.

## Runtime config

User config is stored at `~/.gltronrc` (the `RC_NAME` macro in `configure.ac`).
Some setups also leave a `~/.gltron` directory. Deleting these is the standard
"reset to defaults" trick.

## Tasks / plans (in-flight)

Active work lives under `tasks/` and completed work is moved to
`tasks/archive/<YYYY>/<MM>/<DD>/` so it survives across Claude sessions.

**Reference docs:** `tasks/reference/camera-and-observer-pipeline.md` (added 2026-08-27) —
the per-frame camera dispatch + the post-round observer/recognizer switch; read it before
the camera code (it grounds `post-round-camera-bug.md`). Note: the `modernization-survey`
was reconciled + archived 2026-08-27 (its deletions were already executed).

Active:

- [Promote static analysis to a fatal gate](tasks/lint-fatal-gate.md) — **proposed, needs go-ahead**; drive the advisory clang-tidy/Cppcheck findings to an enforced gate, the static-analysis counterpart to the compiler `-Werror` gate the modernization sweep landed. First decision: which checks to enforce vs disable (teaching-artifact churn risk).
- [Latent bugs surfaced by the C sweep](tasks/latent-bugs-from-c-sweep.md) — **Bug 1 FIXED** (`scripting_CopyStringResult` buffer overflow, e0ce0fb8); **Bug 2 needs a maintainer gameplay decision** (`crashTestPlayers` always returns 0 → wall-test runs even after a player-crash / possible double `EVENT_CRASH`; fixing it changes gameplay — see the task's Option A/B).
- [OpenGL fixed-function → 3.3 core profile port](tasks/opengl-core-profile-port.md) — **not started**, surface at session start and ask Bill before beginning.
- [SDL2 → SDL3 port](tasks/sdl3-port.md) — **shipped 2026-05-08** on Linux; macOS not retested (work remains).
- [Post-round camera bug](tasks/post-round-camera-bug.md) — **fix applied (staged), runtime-unconfirmed**; root cause was glance-state (`bIsGlancing`) leaking into the observer camera on a crash-while-glancing. Runtime confirmation is tracked in [Verify post-round camera fix](tasks/verify-post-round-camera-fix.md) (reported 2026-05-08, diagnosed/fixed 2026-08-03).
- [Verify HandmadeMath audio pitch](tasks/verify-handmademath-audio-pitch.md) — **blocked** on Bill's host listen-test of the SourceEngine pitch effect (the Phase-2 `Vector3 *` → `HMM_DotV3` rewrite).

Completed — one-line pointers; the full post-mortems (rationale, phases, rejected
alternatives, deletions) live in the archived task docs:

- Container build cleanup — `tasks/archive/2026/08/25/container-build-cleanup.md`
- AI freezes on second round — `tasks/archive/2026/04/27/ai-freezes-on-second-round.md`
- Autotools → CMake port — `tasks/archive/2026/04/27/cmake-port.md`
- AppImage build target (host-dependent) — `tasks/archive/2026/04/27/appimage-target.md`
- SDL window architecture cleanup — `tasks/archive/2026/05/10/sdl-window-architecture.md`
- Replace nebu math with HandmadeMath (all 5 phases) — `tasks/archive/2026/05/10/handmademath-port.md` (its one live follow-up is split into `tasks/verify-handmademath-audio-pitch.md`)
- Modernization survey — `tasks/archive/2026/08/27/modernization-survey.md`
- ASan + UBSan(trap) sanitizer gate — `tasks/archive/2026/10/17/add-sanitizer-gate.md`
- Behavior-preserving C modernization sweep (C23 pin, `-Werror` warning gate clean under clang+gcc, 493→0 first-party warnings; nullptr/attributes; no third_party edits) — `tasks/archive/2026/10/17/c-modernization-sweep.md` (follow-up: `tasks/lint-fatal-gate.md`)
- Standardize the container build + Emacs C support (`Makefile.docker`→`Makefile` on the fleet contract, portable `make format`, Emacs clangd/eglot + `make update-emacs-packages` + vendored `elpa/`) — `tasks/archive/2026/10/17/standardize-build-and-emacs.md` (one maintainer GUI check pending: clangd-in-emacs)

When starting a new task, add a one-line entry under "Active" here pointing
at a `tasks/<slug>.md` file, and keep the task file updated with status,
approach, and open questions so the next session can pick it up cold. When a
task is complete, move its file to `tasks/archive/<YYYY>/<MM>/<DD>/` and move
its entry to the "Completed" list.

## Conventions

- **C23** (`CMAKE_C_STANDARD 23`, extensions on → `gnu23`; bumped from C11 by the
  behaviour-preserving modernization sweep 2026-10-17). 2-space indent, formatted with
  the project `.clang-format`. Builds clean under both clang and gcc.
- **Warning gate (enforced).** `gltron_warnings` applies the raised first-party warning
  set (`-Wall -Wextra -Wpedantic -Wshadow -Wvla -Wwrite-strings -Wformat=2
  -Wsign-conversion`, plus C-only and clang-only extras) with `-Werror`, **on by default**
  (`-DGLTRON_WARN=OFF` for a quiet build; `-DGLTRON_WERROR=OFF` to downgrade to warnings).
  Scoped to first-party code only — vendored/system headers come in as `SYSTEM` includes.
  Deliberately **not** `-Wconversion` (dropped int→float) and `-Wno-unused-parameter`
  (callback signatures). A new warning in code you touch is yours to fix at the source.
- **Static analysis is advisory, not a gate.** `tools/lint.sh` runs clang-tidy
  (bug-finding checks) + Cppcheck over first-party sources and always exits 0 — the project
  `.clang-tidy` produces thousands of `readability-*` style findings on this deliberately-old
  code, so it is NOT fatal. The compiler `-Werror` gate above is the enforced tier-(a).
- **Error strategy: `assert(0)` / `assert(cond)` for unreachable/invariant failures**
  (e.g. `src/filesystem/path.c`), and a returned `NULL`/`nullptr` with the owner contract
  stated for allocators (e.g. `getPossiblePath` = caller frees, nullptr on failure). This
  is gltron's chosen convention — don't replace the `assert`s with another scheme.
- Don't add new abstractions or "modernize" code as drive-by cleanup — the
  codebase is intentionally a teaching artifact. Match the surrounding style.
- Prefer minimal, surgical fixes over refactors. If you spot something worth
  cleaning up, mention it rather than doing it.
