# Modernization survey

**Status:** survey doc, no work started. Bill asked 2026-05-08 for an audit
of antiquated pieces of the codebase (libraries, formats, subsystems) with
recommendations on what to replace, drop, or restructure. This is the
inventory; individual items get their own `tasks/<slug>.md` files
when picked up (moved to `tasks/archive/<YYYY>/<MM>/<DD>/` when complete).

The fixed-function OpenGL pipeline is **out of scope** for this survey
(intentionally retained — see [opengl-core-profile-port.md](opengl-core-profile-port.md)
for the eventual modernization, which is gated on Bill's pedagogical goals).

## TL;DR — recommended sequence

Ordered roughly by ratio of (value × confidence) to (risk × effort):

1. **Drop lib3ds** (vendored + linked, but it is dead code today). Pure deletion. **DONE 2026-05-08.**
2. **Delete unused/unbuilt nebu sources** (`extgl.c`, `light.c`, `quad.c`,
   `quadbuf.c`, `renderer_gl.c`, `scene.c`, `benchmark.c`, `png_texture.c`,
   `mesh_3ds.cpp`, `spline.c`). Pure deletion. **DONE 2026-05-08.**
3. **Delete the historical `.dsp`/`.vcproj`/`.sln` and `.sit` files** plus
   `PBProject/`, `CWProject.sit`, `gltron.sln`, `gltron.dsp`, `gltron.dsw`,
   `gltron.vcproj`, `lua5/lua5.dsp`, `lua5/lua5.vcproj`, `nebu/nebu.dsp`,
   `nebu/nebu.vcproj`, `nebutest/nebutest.vcproj`, `win32/`. CMake is the
   only build system now. **DONE 2026-05-08.**
4. **SDL window architecture cleanup** — already planned; surfaces the global
   gWindow before doing anything else SDL- or GL-context-shaped.
   See [sdl-window-architecture.md](sdl-window-architecture.md).
   **DONE 2026-05-08 (Option A — accessors).**
5. **Lua 5.0.2 → Lua 5.4 (system package)** — biggest single modernization win.
   Drops the entire vendored `lua5/` tree, removes deprecated API calls, and
   gives users a current sandbox/GC. ~50 lines of glue code to update.
   **DONE 2026-05-08.**
6. **Reduce nebu surface** — keep what gltron actually uses, drop the rest;
   *do not dissolve nebu into src/*. See item below.
7. **SDL2 → SDL3** — **DONE 2026-05-08** on Linux. See
   [sdl3-port.md](sdl3-port.md). macOS not retested.
8. **GL core profile** — already planned, deferred until Bill is ready.
   See [opengl-core-profile-port.md](opengl-core-profile-port.md).

---

## Findings

### 1. lib3ds is dead code

**Vendored at:** `lib3ds/` (20 .c files, ~6k LOC)
**Linked at:** `src/CMakeLists.txt:31`, `nebu/video/CMakeLists.txt:6`
**Used at:** nowhere in `src/`. The only callers of `nebu_Mesh_3ds_*` are
inside `nebu/video/mesh_3ds.c` itself (verified with grep — no external
references).

The actual model loader the game uses is `gltron_Mesh_LoadFromFile` in
`src/video/model.c:78`, which parses Wavefront OBJ via `zlib`'s `gzgets`
(`.obj.gz` files in `data/`).

**Recommendation:** delete `lib3ds/`, delete `nebu/video/mesh_3ds.{c,cpp}`,
delete `nebu/include/video/nebu_mesh_3ds.h`, drop `lib3ds` from both
CMakeLists and the Dockerfile copy step. No game-side change needed.

**Risk:** none — verified there are no callers.

### 2. Unbuilt nebu source files (already de-facto dead)

`nebu/video/CMakeLists.txt` builds 9 of the 18 source files in that
directory. The unbuilt ones are: `extgl.c`, `light.c`, `mesh.c` (note: the
*built* version is `mesh.c` itself — sorry, this one *is* built; the unbuilt
sibling is `mesh_3ds.cpp`), `quad.c`, `quadbuf.c`, `renderer_gl.c`,
`scene.c`, `benchmark.c`, `png_texture.c` (duplicate of `nebu/base/png_texture.c`),
`mesh_3ds.cpp` (C++ version of `mesh_3ds.c`).

`nebu/base/spline.c` is also present but not in `nebu/base/CMakeLists.txt`.

**Recommendation:** delete all of them and the matching headers under
`nebu/include/video/`. Verify each header has no external `#include` first
(quick grep).

**Risk:** very low — they don't link today, so deleting them changes nothing
at runtime.

### 3. `extgl.c` specifically — pre-GLEW GL extension loader

`nebu/video/extgl.c` is a 2001 Lev Povalahev GL extension loader. We use
GLEW now (`nebu/include/video/nebu_renderer_gl.h:5`). The file isn't built
but is still in tree.

Bundled with item #2.

### 4. Lua 5.0.2 (vendored) → Lua 5.4 (system package)

**Vendored at:** `lua5/` — confirmed `LUA_VERSION "Lua 5.0.2"` in
`lua5/include/lua.h`. That release shipped in 2003. Current Lua is 5.4.x.

The wrapper `nebu/scripting/scripting.c` calls a number of APIs that have
been **removed** since 5.0:

| Used here | Status in 5.4 | Replacement |
| --- | --- | --- |
| `lua_open()` | removed | `luaL_newstate()` |
| `luaopen_base/table/string/io(L)` | removed | `luaL_openlibs(L)` |
| `lua_dofile(L, name)` | removed | `luaL_dofile(L, name)` |
| `lua_dostring(L, s)` | removed | `luaL_dostring(L, s)` |
| `lua_strlen` | removed | `lua_rawlen` |
| `luaL_getn` | removed | `lua_rawlen` |
| `lua_setgcthreshold` | removed | `lua_gc(L, LUA_GCSTEP, n)` |
| `lua_pushnumber(L, intVal)` | works but lossy | `lua_pushinteger(L, n)` |

Plus `src/game/scripting_interface.c` will need the same treatment.

The Lua scripts themselves (`scripts/*.lua`, `levels/*.lua`, ~2300 LOC) use
mostly basic features — string concatenation, tables, `ipairs`/`pairs`, a
few `string.format` calls. They will need a once-over for 5.0→5.4 changes
(e.g. `table.getn(t)` → `#t`, `arg` table semantics, `setfenv`/`getfenv`
gone in 5.2+, `math.mod` → `math.fmod`). Most of those probably aren't
present; a grep + smoke test will tell.

**Why not just stay on 5.0:**
- Distros stopped shipping it ~2010. Vendoring is the only option.
- Twenty-three years of bug fixes, GC improvements, and security work missed.
- The bundled `lua5/lib/loadlib.c` exposes dlopen-style C extension loading
  to scripts; we don't need it (we don't load Lua C modules from disk) and
  it's a sandbox liability.

**Why Lua specifically (vs. replacing Lua entirely):** the scripts are
config + menu + HUD layout + level descriptions. Lua excels at that and the
existing scripts are written cleanly. **Don't replace Lua with another
scripting language.** The cost (rewriting 2.3k LOC of working scripts)
massively outweighs any benefit.

**Alternative considered: LuaJIT.** Faster, but no upside here — the game
spends ~no time in scripts during the hot loop, and LuaJIT lags Lua's
language version (it's effectively 5.1 + extensions). Skip.

**Recommendation:** delete `lua5/` entirely, switch CMake to
`pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua>=5.4)` (or
`find_package(Lua 5.4)`), update `nebu/scripting/scripting.c` and
`src/game/scripting_interface.c`, smoke-test each script.

**Effort:** medium. The C glue rewrite is ~1 hour. The Lua-side
compatibility check + fixes is the unknown — could be 1 hour or 1 day.

### 5. `luaopen_io` — keep it

`nebu/scripting/scripting.c:23` opens Lua's `io` library on the embedded
state. **Initial pass thought this could be dropped — wrong.** I grepped
only the C code; the Lua scripts use `io.write` extensively for status
logging (`scripts/basics.lua`, `main.lua`, `menu.lua`, `menu_functions.lua`,
`save.lua`, `video.lua`). Removing `luaopen_io` segfaults the game on
startup (artpack enumeration in `video.lua` is the first thing to die).

**Recommendation:** leave it. If sandboxing ever matters, narrow to
`io.write`/`io.flush` rather than dropping the whole library.

### 6. Nebu — what to keep, what to cut

Original-author intent was a generic engine layer ("Nebu"). In practice the
only consumer is gltron, and large parts have already rotted. Bill's
direction is **shrink, don't dissolve** — keep nebu as the helper layer,
just trim it.

| Subsystem | LOC | Used by gltron? | Recommendation |
| --- | --- | --- | --- |
| `nebu/base/` | small (~10 files) | yes — math, lists, random, surface, system | **keep**; drop `spline.c` (unbuilt). |
| `nebu/filesystem/` | small | yes — `nebu_FS_GetPath`, `file_open`/`file_gets` | **keep**. Thin POSIX/SDL_RWops wrapper. |
| `nebu/input/` | tiny | yes — keyname tables, event pumping | **keep**, will need SDL3-shaped renames later. |
| `nebu/scripting/` | 1 file | yes | **keep**, see Lua item. |
| `nebu/video/` | mixed | partial — `nebu_2d`, `nebu_Font`, `video_system`, `nebu_Mesh`, `nebu_console` (only `console_Seek`) | **trim** — see item #2. After cleanup, this becomes coherent. |
| `nebu/audio/` | 7 .cpp + 1 .c | yes — `Sound::System`, `SourceMusic`, `SourceSample`, `Source3D`, `SourceEngine` via `src/audio/sound_glue.cpp` | **keep but reconsider**, see audio item below. |

The "C++ inside a C codebase" footprint is essentially `nebu/audio/*.cpp`
plus the glue file `src/audio/sound_glue.cpp`. That's the only reason
gltron is built as `LINKER_LANGUAGE CXX`. If the audio engine were rewritten
in C (or replaced by SDL_mixer), gltron would become a pure-C project.

**Recommendation:** after #1 + #2 land, the remaining nebu surface is
defensible and worth keeping.

### 7. The Nebu audio engine — pre-OpenAL software mixer

`nebu/audio/` is a hand-rolled software mixer with a `Sound::System` that
locks an `SDL_AudioSpec` and mixes a list of `Sound::Source`/`Source3D`
objects in a callback. It does its own Doppler + per-source pitch shifting
in `Source3D.cpp` and `SourceEngine.cpp` (the lightcycle engine pitch is
generated this way — that's *the* audio detail of the game).

That's neat and pedagogically interesting (per-sample mixing, custom
Doppler), but it predates SDL_mixer becoming usable and predates OpenAL
becoming ubiquitous. Today the equivalent is:

- **SDL_mixer / SDL3_mixer** — fine for samples and music, but no built-in
  3D positional audio. Doppler/pitch would still be hand-rolled.
- **OpenAL Soft** — purpose-built for positional audio + Doppler, mature,
  cross-platform, single dependency.
- **miniaudio** (single-header) — lighter than OpenAL, supports 3D + pitch.
- **Keep nebu_audio** — it works, the engine pitch effect is custom and
  bespoke, and replacing it changes the game's *sound*, not just its
  plumbing.

**Recommendation:** **don't touch this in the near term.** The custom
SourceEngine pitch shifting is a feature of the game, not a bug, and the
risk-of-regression is high for a tiny modernization payoff. Revisit only
if SDL2_sound becomes painful (e.g. SDL3 transition), in which case
OpenAL Soft is the path of least surprise.

### 8. SDL2_sound dep — fine but worth flagging

`SDL2_sound` is the audio decoder front-end (loads `.wav`, `.ogg`, `.it`).
The `.it` (Impulse Tracker) decode goes through libmikmod (visible in the
Dockerfile dependencies — `mikmod`, `mikmod-devel`). **DONE 2026-05-08**:
swapped to SDL3_sound as part of the SDL3 port; the renames were not just
cosmetic — `Sound_AudioInfo` was replaced with `SDL_AudioSpec`, and the
RWops-taking `Sound_NewSample` calls were rewritten to
`Sound_NewSampleFromFile`. Captured in [sdl3-port.md](sdl3-port.md).

### 9. SDL2 → SDL3

**DONE 2026-05-08** on Linux. See [sdl3-port.md](sdl3-port.md) for the
full landed change set and the macOS / interactive-smoke-test follow-ups.

### 10. GLEW

We use GLEW for GL extension loading (`nebu/include/video/nebu_renderer_gl.h`).
For pure fixed-function (i.e. before the core profile port), GLEW is
overkill — we only use `GLEW_ARB_multitexture` (`nebu/video/video_system.c:139,148`).
For the core profile port, GLEW still works but is unmaintained-ish; the
modern equivalent is **glad** (header + generator, no runtime dep).

**Recommendation:** keep GLEW until the GL core profile port. Bundle a
GLEW→glad swap into that work, *not* this survey's batch.

### 11. `.ftx` (font index) and `.fbmp` formats

`data/babbage.ftx` and `data/xenotron.ftx` are tiny ASCII manifests — line
1: `nTextures texWidth charWidth`; line 2: `firstChar lastChar`; line 3:
font name; lines 4..N: PNG paths for the page textures. Loaded by
`nebu_Font_Load` in `nebu/video/font.c`. The textures are PNGs.

`data/test.fbmp` exists but I see no consumer in the codebase — looks like
an abandoned format. Might just be `.ftx` under another name.

**Recommendation:** the format works, it's literally five lines of `sscanf`.
Replacing it with stb_truetype (runtime TTF rasterization) or SDL_ttf would
be a quality win (resolution-independent text), but it's a UX enhancement,
not a modernization. **Skip unless a UX reason arises.** Delete `test.fbmp`
since nothing reads it.

### 12. `.obj.gz` model loader

`src/video/model.c:78` is a hand-rolled Wavefront OBJ parser using `gzgets`.
Modern alternative: tinyobjloader (single-header). But: the existing parser
works, the assets are simple, and there's no real upside. **Skip.**

### 13. Historical IDE project files

The repo carries old Visual C++/Xcode/CodeWarrior project files that no
longer reflect reality:

- `gltron.dsp`, `gltron.dsw`, `gltron.vcproj`, `gltron.sln` — VS6/VS2003 era
- `lua5/lua5.dsp`, `lua5/lua5.vcproj`
- `nebu/nebu.dsp`, `nebu/nebu.vcproj`
- `nebutest/nebutest.vcproj`
- `CWProject.sit` (CodeWarrior project, StuffIt-compressed)
- `PBProject/` (Project Builder, pre-Xcode)
- `win32/` (legacy Win32 build helpers — verify before deleting)

CMake is the only path now. **Recommendation:** delete the obviously dead
ones in one PR. Keep `win32/` only if it contains assets/icons/manifests
referenced by the CMake build (do a quick check before removing).

### 14. `nebutest/`

`nebutest.c`, `quake-movement.c`, `video.c` plus a `models/` subdir. Looks
like a separate test harness for the Nebu engine layer, not built by our
CMakeLists. Either revive (fold into `add_test()` somehow) or delete.

**Recommendation:** delete unless Bill remembers using it. It hasn't been
maintained alongside the SDL2 port and almost certainly doesn't compile.

---

## Items I considered and explicitly rejected

- **Replace Lua with another scripting language** — rejected; rewriting
  2.3k LOC of working scripts has no upside.
- **Replace nebu_audio with SDL_mixer right now** — rejected; the
  SourceEngine pitch shift is the game's signature audio.
- **Replace GLEW now** — rejected; bundle into the GL core profile port.
- **Replace libpng with stb_image** — not surveyed in detail, but: libpng
  works, is everywhere, and has zero ergonomic cost in our wrapper at
  `nebu/base/png_texture.c`. Skip.
- **Dissolve nebu into src/** — rejected per Bill's direction.

---

## Verification, when items get implemented

For each item, the smoke test is the same: `make -f Makefile.docker build`
must succeed, and the resulting binary must launch the menu, start a
2-player AI match, run for 30s without an audio glitch or visual
regression, and `ESC` back to menu cleanly. The Lua and nebu items in
particular touch enough surface that "it links" is not enough — actually
play the game.
