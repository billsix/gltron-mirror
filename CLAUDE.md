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
- A Fedora-based **podman** dev container is provided (`Makefile.docker`,
  `Dockerfile`); `make -f Makefile.docker image` builds the image and
  compiles gltron at image-build time, `make -f Makefile.docker shell`
  drops into a shell, and `make -f Makefile.docker build` re-runs the
  CMake build against the mounted source tree (writes to `./bld/`).
  CMake natively emits `compile_commands.json`. `clang-tidy` and
  `clang-format` are configured (`.clang-tidy`, `.clang-format`).

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

## Runtime config

User config is stored at `~/.gltronrc` (the `RC_NAME` macro in `configure.ac`).
Some setups also leave a `~/.gltron` directory. Deleting these is the standard
"reset to defaults" trick.

## Tasks / plans (in-flight)

Detailed plans live under `docs/plans/` so they survive across Claude sessions.
Index:

- [AI freezes on second round](docs/plans/ai-freezes-on-second-round.md) — **fixed**, see plan for explanation.
- [Port build system from autotools to CMake](docs/plans/cmake-port.md) — **draft**, awaiting answers to open questions.
- [AppImage build target](docs/plans/appimage-target.md) — **shipped (host-dependent)**; future work to make the AppImage self-contained is captured in the plan.
- [SDL window architecture cleanup](docs/plans/sdl-window-architecture.md) — **shipped** (Option A, accessors).
- [SDL2 → SDL3 port](docs/plans/sdl3-port.md) — **shipped 2026-05-08** on Linux; macOS not retested.
- [OpenGL fixed-function → 3.3 core profile port](docs/plans/opengl-core-profile-port.md) — **not started**, surface at session start and ask Bill before beginning.
- [Modernization survey](docs/plans/modernization-survey.md) — **survey doc** (2026-05-08); inventory of antiquated libraries/subsystems with a recommended sequence. Read before picking any "modernize X" task.
- [Replace nebu math with HandmadeMath](docs/plans/handmademath-port.md) — **all 5 phases shipped 2026-05-08**. Linear algebra now lives in `<HandmadeMath.h>` (vendored at upstream `661fef0`, exposed via the `handmademath` CMake `INTERFACE` target). The surviving geometric helpers (`segment2_*`, `box2_*`, `box3_*`, `vec2_Orthogonal`, `vec3_TriNormalDirection`, `uintFromVec3`) live in `nebu/include/base/nebu_geom.{h,c}`. `vec2`/`vec3`/`matrix` are documented typedef aliases for the HMM types in that same header (kept for readability — drop if the inconsistency bites). C standard bumped 99 → 11. Files deleted along the way: `nebu/base/{vector,matrix,quat,camera}.c`, `nebu/include/base/{nebu_vector,nebu_matrix,nebu_quat,nebu_Vector3}.h`, `nebu/include/video/nebu_camera.h`, `src/game/game_level.c`. **Outstanding:** Bill should listen-test the SourceEngine pitch effect on his host to validate the Phase-2 audio rewrite (`Vector3 *` → `HMM_DotV3`).
When starting a new task, add a one-line entry here pointing at a
`docs/plans/<slug>.md` file, and keep the plan file updated with status,
approach, and open questions so the next session can pick it up cold.

## Conventions

- C99, 2-space indent, formatted with the project `.clang-format`.
- Don't add new abstractions or "modernize" code as drive-by cleanup — the
  codebase is intentionally a teaching artifact. Match the surrounding style.
- Prefer minimal, surgical fixes over refactors. If you spot something worth
  cleaning up, mention it rather than doing it.
