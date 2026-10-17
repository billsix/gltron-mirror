# Add an ASan + UBSan(trap) build gate

**Status:** in progress — started 2026-10-17 (gate wired; authoritative image build pending)
**Created:** 2026-06-16
**Started:** 2026-10-17 (William Emerison Six <billsix@gmail.com>) — gate-first for
`tasks/c-modernization-sweep.md` (its decision 4)

## Decisions (settled 2026-10-17 — the no-source-edit route; resolves the three
## "decisions for Bill" below)

1. **Xvfb + software GL**, not the build-only floor. **Proven feasible first:** a
   probe in the image showed `xvfb-run -s "-screen 0 1024x768x24" glxinfo` under
   `LIBGL_ALWAYS_SOFTWARE=1` reports `llvmpipe … OpenGL 4.5 (Compatibility Profile)`,
   which covers gltron's fixed-function needs.
2. **`timeout`-based bounded run — NO source hook.** The `GLTRON_FRAME_LIMIT` hook in
   `timedemo.c`/`gltron.c` is *not* added (teaching-artifact rule: no drive-by source
   edits; that hook still needs explicit go-ahead). Instead the gate gets real
   game-logic coverage (AI/physics/camera/render, not just the menu) by enabling
   **timedemo mode in the THROWAWAY install tree only** — `sed 's/^timedemo = nil/
   timedemo = 1/'` on the gate's own installed `scripts/main.lua`, never the committed
   source. A clean `timeout` kill is a pass.
3. **Env `ASAN_OPTIONS=detect_leaks=0`** (+ `abort_on_error=1`), not a source
   `__asan_default_options`. No source edit; LSan off so the gate targets corruption,
   not gltron's by-design exit leaks. A `tools/asan-gate.supp` is wired in if present
   (none committed yet — add only if a run shows Mesa/SDL/Lua noise).

## Progress (2026-10-17)

- **Feasibility de-risked:** headless software GL confirmed (decision 1). The stale
  prebuilt image was Fedora 43 / clang 21 and predated SDL3, so a local sanitizer
  build couldn't run against it → **rebuilding the image to Fedora 44 first** (the
  current `Dockerfile` already targets `fedora:44`; clang 22 / gcc 16 / SDL3 present).
- **Gate implemented (staged, not yet verified by a full build):**
  - `tools/sanitizer-gate.sh` — builds gltron under UBSan-trap then ASan, installs
    each to a private prefix with its own data tree, flips timedemo on in that tree,
    runs headless under Xvfb+llvmpipe with a `timeout` cap, and classifies exit
    ({0,124,143}=pass; SIGILL/SIGABRT/other=fail). Multi-step failure propagation per
    the shared shell-gate convention; portable (SRC_DIR arg, no hardcoded abs paths).
  - `entrypoint/02-install-graphics.sh` — adds `xorg-x11-server-Xvfb`.
  - `Dockerfile` — `ARG RUN_SANITIZERS=0` + a guarded gate RUN after the main build.
  - `Makefile.docker` — `RUN_SANITIZERS ?= 1`, threaded into the `image` target.
- **Pending:** run `make -f Makefile.docker image RUN_SANITIZERS=1` end-to-end to
  confirm both sanitizer builds compile/link and the headless timedemo run is clean
  (or surfaces real findings). ASan-through-uninstrumented-Mesa may need
  `-shared-libasan`/a supp file — handle if the build surfaces it.

## Goal

Extend the `make -f Makefile.docker image` build so it also compiles gltron
under **UBSan trap mode** (`-fsanitize=undefined -fsanitize-trap=undefined`) and
**ASan** (`-fsanitize=address`), exercises the binary, and **fails the image** on
any undefined behavior or memory error. Same idea as spimulator's gate (see its
archived primer/rationale at
`/billopt/spimulator/tasks/archive/2026/06/16/ubsan-sweep.md`, and the wiring
described in `/billopt/spimulator/CLAUDE.md` under "Sanitizer gate"). Opt-in via a
`RUN_SANITIZERS` build arg defaulting to `1`, mirroring spimulator.

**Why:** catch signed-overflow / bad-shift UB (which is benign at `-O0` but a
miscompile landmine at `-O2`/`-O3`) and memory-safety bugs (overflow,
use-after-free) before they ship. Trap mode needs no sanitizer runtime to link,
so it's low-friction in the Fedora image; trap mode is the *reliable* gate
(spimulator found diagnostic UBSan under-reports — see its doc).

## The CMake-specific way to add the flags

gltron is **CMake (≥3.21) + Ninja**, not Meson, so spimulator's `-Dc_args=` knobs
don't apply. Use a separate out-of-tree build dir per sanitizer, passing the flags
through the cache. The cleanest per-config approach is a custom build type so the
flags land on **both compile and link** (ASan/UBSan must be on the link line too):

```sh
# UBSan (trap) build
cmake -S /gltron -B /tmp/san-ubsan -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=undefined -fsanitize-trap=undefined" \
      -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fsanitize-trap=undefined" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined -fsanitize-trap=undefined"
cmake --build /tmp/san-ubsan -j

# ASan build
cmake -S /gltron -B /tmp/san-asan -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
cmake --build /tmp/san-asan -j
```

Notes:
- The project's CMakeLists has **CXX enabled** (`project(... C CXX)`), so set both
  `CMAKE_C_FLAGS` and `CMAKE_CXX_FLAGS`.
- These flags are *appended* to whatever the build type sets, so `Debug` (`-g -O0`)
  is fine. Don't reuse `GLTRON_WARN`'s `-Werror` here — sanitizer builds can warn.
- Use **clang** (`CC=clang CXX=clang++`, the image's default) to match how the
  rest of the image builds, and so trap mode behaves as in spimulator.
- Scope is the whole `gltron` target plus `nebu` and vendored single-header math —
  unlike spimulator there is **no `-nostdlib` sub-target to exclude**, so a global
  flags pass is acceptable. (HandmadeMath is a header-only `INTERFACE` lib and gets
  instrumented wherever it's included, which is fine.)

## The BIG caveat — gltron needs a display/GPU, and has no real test

This is the **central feasibility blocker** and must be solved (or the gate scoped
down) before the gate is worth anything:

- **`main()` always opens a window + GL context.** `main` →
  `initSubsystems` (`src/game/init.c`) → `initVideo` → `nebu_Video_Init`
  (`nebu/video/video_system.c`) calls `SDL_Init(SDL_INIT_VIDEO)`,
  `SDL_CreateWindow`, `SDL_GL_CreateContext`, then GLEW init. There is **no
  non-GL code path** that reaches the interesting game logic — even "timedemo"
  mode (set via `timedemo = 1` in `scripts/main.lua`, or the `c_timedemo()` menu
  action) runs through full video init and `displayGame()`/`changeDisplay()`.
  So a sanitized run requires a working GL context.

- **The existing CTest is a no-op.** `CMakeLists.txt` (the `if(BUILD_TESTING)`
  block, the `test(` reference the scan saw) adds exactly one test,
  `gltron_binary_exists`, whose command is `cmake -E true` — it only asserts the
  binary linked and exists. It executes **zero game code**, so running `ctest`
  under sanitizers exercises nothing. A comment in that block already admits the
  game `assert(argc == 1)`s and needs a display, so no functional smoke test
  exists today.

- **The in-tree leak checker is a Windows-only no-op.**
  `nebu/base/debug_memory.c`'s `nebu_debug_memory_CheckLeaksOnExit()` is empty
  except under `_DEBUG` (MSVC). It does not help ASan/LSan.

### Feasibility verdict

**Gltron CAN be exercised non-interactively, but only via a software/offscreen
GL stack — there is no display-free code path, so a bare headless run won't
work.** Two viable routes, in order of preference:

1. **Xvfb + Mesa software rendering (recommended, lowest-risk).** Run the binary
   under a virtual X server with llvmpipe:
   ```sh
   LIBGL_ALWAYS_SOFTWARE=1 SDL_VIDEODRIVER=x11 \
     xvfb-run -a /tmp/san-ubsan/.../gltron   # plus an auto-exit, see below
   ```
   The image already installs `mesa-dri-drivers` and `mesa-demos`; it would need
   **`xorg-x11-server-Xvfb`** added to the Dockerfile dnf list (temporary dev
   dep per the standing arrangement, or permanent if we keep the gate). SDL3's
   `dummy` video driver is **not** enough on its own — it provides no GL context,
   so `SDL_GL_CreateContext`/GLEW would fail.

2. **EGL surfaceless / OSMesa offscreen.** Heavier to wire; only pursue if Xvfb
   proves flaky.

### Making the run terminate non-interactively

The game loops until the user quits, so the gate needs a bounded run. Options
(pick the smallest):
- **Add a hidden "run N frames then exit" path** — e.g. honor an env var
  (`GLTRON_FRAME_LIMIT`) checked in the timedemo idle loop
  (`src/game/timedemo.c:idleTimedemo`, which already counts `frames`) and call
  `nebu_System_ExitLoop(...)` once the count is hit; have the image set
  `timedemo = 1` (or a dedicated headless flag) so it boots straight into
  timedemo. This is the cleanest "automated run" and exercises AI, movement,
  physics, camera, and the render path. **Requires a small, surgical source
  addition** — flag it to Bill before writing it (teaching-artifact rules: no
  drive-by refactors).
- **Cheaper interim:** run the binary under `timeout 10s` and treat a sanitizer
  abort (SIGILL for UBSan-trap, ASan's non-zero abort) as failure while
  tolerating the `timeout` kill (exit 124) as success. Less precise (a clean
  10 s may miss later UB), but needs **no source change** and still smoke-tests
  startup + steady-state rendering under the sanitizers.

### Honest minimum if the display route is rejected

If a software-GL run is deemed not worth the Dockerfile weight, the gate
degrades to **"sanitized *build* only"** — compile both sanitizer configs and
fail on compile/link errors, then run the no-op `ctest`. That still catches
sanitizer-incompatible code and link regressions, but exercises **no runtime
UB/ASan checks**, so it is far weaker than spimulator's (which has a real test
suite to run under the sanitizers). Be explicit that this is the floor, not the
goal.

## Suppressions (SDL3 / GL / Mesa / Lua)

Third-party libraries linked into the binary (SDL3, SDL3_sound, GLEW, Mesa
llvmpipe, libpng, system Lua 5.4) are **not** instrumented (no `-fsanitize` on
their prebuilt objects), but ASan still sees leaks/interceptable calls through
them and Mesa software rendering is a known source of one-time allocations
reported as "leaks." Plan for:
- An **ASan suppression file** (`ASAN_OPTIONS=suppressions=/gltron/tools/asan.supp`)
  and/or **LSan suppressions** (`LSAN_OPTIONS=suppressions=...`) keyed on
  `libGL`, `swrast`, `llvmpipe`, `libSDL3`, `lua` frames if they produce noise.
- UBSan trap mode produces no such library noise (it only traps in *our*
  instrumented code), which is another reason it's the better hard gate.

## ASan leaks

gltron is leaky-at-exit by design (it's a game; `exitSubsystems` frees some but
not all, and the in-tree leak checker is a Windows no-op). Default LSan off so the
gate flags **corruption**, not intentional exit leaks — mirror spimulator: add a
weak, ASan-guarded

```c
#if defined(__SANITIZE_ADDRESS__) || \
    (defined(__has_feature) && __has_feature(address_sanitizer))
const char *__asan_default_options(void) { return "detect_leaks=0"; }
#endif
```

near `main()` in `src/gltron.c`. (Alternatively set `ASAN_OPTIONS=detect_leaks=0`
in the gate's run command — no source change — which is preferable here given the
teaching-artifact "no drive-by edits" rule. Decide which.) If we later *want* leak
coverage, turn LSan on with the suppression file above.

## Wiring into the image (mirror spimulator)

- Add `ARG RUN_SANITIZERS=0` to the `Dockerfile` (default 0 in Dockerfile per the
  family contract; `Makefile.docker` passes `1`).
- Add a `RUN if [ "$RUN_SANITIZERS" = "1" ]; then ... fi` block **after** the
  existing build/install/ctest step, doing: UBSan-trap configure+build+run, then
  ASan configure+build+run, `set -e` so any abort fails the image, `rm -rf` the
  temp build dirs at the end.
- In `Makefile.docker`: add `RUN_SANITIZERS ?= 1` and thread
  `--build-arg RUN_SANITIZERS=$(RUN_SANITIZERS)` into the `image` target
  (alongside the existing `--build-arg USE_GRAPHICS=...`).
- If the Xvfb/software-GL run route is chosen, add `xorg-x11-server-Xvfb` (and
  confirm `mesa-dri-drivers` covers llvmpipe) to the dnf install list.

## In-container only

All build/run work happens inside the gltron podman image, per the working
arrangement. Trap-mode UBSan needs no extra packages; ASan's runtime ships with
clang. The only possible new dep is Xvfb (above) for the headless run — add it as
a tracked temporary (or permanent, if the gate is kept) Dockerfile change.

## Open questions / decisions for Bill — RESOLVED 2026-10-17 (see Decisions, top)

1. Xvfb + software-GL run — **yes** (proven feasible), not the build-only floor.
2. Frame-limit source hook — **no**; use the `timeout` interim + timedemo-in-the-
   install-tree. The source hook remains available as a future upgrade if deeper,
   deterministic coverage is wanted (would need go-ahead).
3. Leak handling — **env `ASAN_OPTIONS`**, no source edit.

## Acceptance

- `make -f Makefile.docker image` (with `RUN_SANITIZERS=1`, the default) builds
  gltron under both UBSan-trap and ASan and **fails on any UB/memory error**.
- `make -f Makefile.docker image RUN_SANITIZERS=0` skips the gate (fast path).
- Whatever runtime exercise is chosen (headless automated run, or honest
  build-only floor) is documented here, and the run terminates deterministically.
- Any suppressions added are committed under `tools/` and referenced from the
  gate's run command.
- LSan/leak decision recorded; ASan run does not fail on intentional exit leaks.
- Decision recorded on whether the gate (and any Xvfb dep / source hook) is kept
  permanently or was a one-off audit.
