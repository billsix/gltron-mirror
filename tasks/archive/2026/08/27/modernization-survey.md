# Modernization survey

**Status:** DONE / reconciled 2026-08-27 (William Emerison Six <billsix@gmail.com>). The survey's
recommendations are executed; the stale "no work started" header was wrong — items were completed
2026-05-08 and marked DONE inline. Archived as a lean record.

## What the survey recommended, and where it landed

- **Deletions — all executed (verified 2026-08-27: `lib3ds/`, `lua5/`, `nebutest/`, `win32/`,
  `PBProject/` and the orphaned `nebu/video/*` sources are gone):**
  1. Drop `lib3ds` (dead code) — DONE 2026-05-08.
  2. Delete unbuilt nebu sources (`extgl.c`, `light.c`, `quad.c`, `quadbuf.c`, `renderer_gl.c`,
     `scene.c`, `benchmark.c`, `png_texture.c`, `mesh_3ds.cpp`, `spline.c`) + `extgl.c` (pre-GLEW loader)
     — DONE 2026-05-08.
  3. Delete historical `.dsp`/`.vcproj`/`.sln`/`.sit` + `win32/`/`PBProject/` (CMake is the only build
     system) — DONE 2026-05-08.
  - **Residual cleaned up now:** `data/test.fbmp` (verified unreferenced) — deleted 2026-08-27.
- **Ports — done, tracked by their own tasks:** SDL window architecture cleanup (accessors), Lua
  5.0.2 → 5.4 (system package), SDL2 → SDL3 (Linux; macOS not retested) — all DONE 2026-05-08. See the
  archived `sdl-window-architecture.md` / `sdl3-port.md`.
- **Deferred, still active:** GL fixed-function → **core profile** — intentionally deferred, gated on the
  maintainer's pedagogical goals. Lives in its own active task `tasks/opengl-core-profile-port.md`.
- **If more "reduce nebu surface" work is wanted** beyond the executed deletions (item 6), spin a fresh
  `tasks/<slug>.md` — it is no longer tracked here.

The full original inventory (per-item findings + file:line risk notes) is preserved in this doc's git
history. Lean archived record per convention.
