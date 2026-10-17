# ASan + UBSan(trap) build gate

**Status:** DONE 2026-10-17 (William Emerison Six <billsix@gmail.com>)
**Created:** 2026-06-16

## What was done

The `make -f Makefile.docker image` build now compiles gltron a second and third
time — under **UBSan trap mode** (`-fsanitize=undefined -fsanitize-trap=undefined`)
and **ASan** (`-fsanitize=address`) — exercises the binary headless, and **fails the
image** on any undefined behavior or memory error. It is opt-in via a
`RUN_SANITIZERS` build arg: default `0` in the `Dockerfile` (fast path), `1` in
`Makefile.docker` (so a plain `make image` runs the gate; `make image
RUN_SANITIZERS=0` skips it). Modeled on spimulator's gate
(`/foo/opt/spimulator` → `tasks/archive/2026/06/16/ubsan-sweep.md`), adapted from
Meson to gltron's CMake build.

**Why:** UB (signed overflow, bad shifts) is benign at `-O0` but a miscompile
landmine at `-O2`/`-O3`; ASan catches overflow/use-after-free. Trap mode needs no
sanitizer runtime to link, so it is the reliable hard gate (spimulator found
diagnostic UBSan under-reports); ASan's runtime ships with clang.

**Verification (2026-10-17, Fedora 44 image, clang 22):** both lanes built, ran the
full headless timedemo, and came back clean — `[ubsan] PASS`, `[asan] PASS`, no
`runtime error` / `AddressSanitizer` / SIGILL signatures. So gltron is UB- and
memory-clean on the timedemo path (startup, video/font/texture load, AI, physics,
movement, camera, HUD, render). (Clang's *default* compile warnings — `-Wpointer-sign`
on the GL `GLuint*` calls, a real `dirsetup.h` header-guard typo, `-Wstring-plus-int`
on `path.c`'s `PREF_DIR + 1`/`SNAP_DIR + 1` — surfaced but do not fail the gate; they
are findings for `tasks/c-modernization-sweep.md`, not this task.)

## Files

- `tools/sanitizer-gate.sh` — the gate. For each sanitizer config: `cmake` configure
  (clang, `Debug`, flags on **compile AND link** via `CMAKE_{C,CXX}_FLAGS` +
  `CMAKE_EXE_LINKER_FLAGS`), build, install to a private prefix with its own
  `GLTRON_DATA_DIR`, run headless, classify the exit. Multi-step failure propagation
  (`status=0; … status=1; exit $status`) per the shared shell-gate convention;
  portable (`SRC_DIR` arg, no hardcoded absolute paths). `GLTRON_GATE_SECONDS` tunes
  the run cap (default 25).
- `Dockerfile` — `ARG RUN_SANITIZERS=0`, and a guarded `RUN … bash
  /gltron/tools/sanitizer-gate.sh /gltron` after the main build/install/ctest.
- `Makefile.docker` — `RUN_SANITIZERS ?= 1`, threaded into `image` as a `--build-arg`.
- `entrypoint/02-install-graphics.sh` — adds `xorg-x11-server-Xvfb` (the gate's only
  new dep; it rides with the graphics group, so `RUN_SANITIZERS=1` needs
  `USE_GRAPHICS=1`, which the script checks for and errors clearly without).

## How the feasibility blockers were solved

gltron has **no display-free code path** (`main` → `initSubsystems` → `initVideo` →
`nebu_Video_Init` always `SDL_CreateWindow` + `SDL_GL_CreateContext` + GLEW) and **no
real functional test** (the sole CTest is `cmake -E true`). So the gate needs a real,
software GL context and a bounded non-interactive run:

- **Software GL headless:** Xvfb + Mesa llvmpipe. Confirmed: under
  `LIBGL_ALWAYS_SOFTWARE=1 SDL_VIDEODRIVER=x11` inside `xvfb-run -s "-screen 0
  1024x768x24"`, Mesa reports `llvmpipe … OpenGL 4.5 (Compatibility Profile)` — fine
  for fixed-function gltron. SDL3's `dummy` driver is **not** enough (no GL context).
  `SDL_AUDIODRIVER=dummy` avoids an audio-device hang (harmless `buffer underrun!`
  spam).
- **Bounded run, no source edit:** `timeout -s TERM` caps the run; a clean timeout
  (exit 124/143) or a clean exit (0) is a PASS, any signal (SIGILL=132 UBSan trap,
  SIGABRT=134 ASan) or other nonzero is a FAIL. xvfb-run's own exit status is
  unreliable, so the real inner status is captured via a temp rc file.
- **Real game-logic coverage, no source edit:** the gate enables timedemo mode by
  `sed 's/^timedemo = nil/timedemo = 1/'` on the installed `scripts/main.lua` **in the
  throwaway install tree only** — never the committed source — so the run drives AI /
  physics / camera / render, not just the menu.

## Decisions (the three "for Bill" questions, resolved the no-source-edit way)

1. **Xvfb + software-GL run**, not the sanitized-build-only floor (feasibility proven
   first).
2. **`timeout` bound, no `GLTRON_FRAME_LIMIT` source hook.** The teaching-artifact
   rule forbids drive-by source edits; the timedemo-in-install-tree trick gave the
   needed coverage without one. *Deferred upgrade (needs go-ahead):* a hidden
   frame-limit/exit hook in `timedemo.c`/`gltron.c` would make the run deterministic
   (exact N frames) rather than wall-clock-bounded — worth it only if a flaky timeout
   or a missed-late-UB gap shows up.
3. **Env `ASAN_OPTIONS=detect_leaks=0:abort_on_error=1`**, not a source
   `__asan_default_options`. LSan off so the gate targets corruption, not gltron's
   by-design exit leaks. No suppression file was needed (Mesa/SDL/Lua produced no ASan
   noise with leaks off); the gate wires in `tools/asan-gate.supp` automatically if one
   is ever added.

## Follow-ups

- **required:** none — the gate is live and green.
- **recommended:** the compile warnings above are real modernization findings; they
  belong to `tasks/c-modernization-sweep.md` (this gate is that sweep's verification
  net — its decision 4).
- **optional:** the deterministic frame-limit hook (decision 2), if wall-clock
  bounding ever proves insufficient.
