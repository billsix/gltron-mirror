# Port build system from autotools to CMake

**Status:** Implemented and verified to build, install, and pass the
ctest smoke check on Fedora 44 with SDL2 2.32, SDL2_sound 2.0.5, GLEW 2.2,
gcc 16, CMake 4.3 (2026-04-28). Awaiting user verification on his Linux
desktop and inside his Fedora 43 podman container, plus the macOS build.

## Decisions (Bill, 2026-04-27)

1. CMake minimum: any reasonable modern version. → Picking **3.21**.
2. `--disable-sound` is broken; **drop the option**. SDL2_sound becomes a hard requirement.
3. Drop `--enable-localdata`. Data will live under the install prefix; users
   run `cmake --install`.
4. Drop `--enable-profile`.
5. After CMake is verified, **delete** `configure.ac`, `autogen.sh`, all
   `Makefile.am` files, `src/include/base/config.h.in`. Don't keep both
   side-by-side.
6. **Build at image-build time** in `Dockerfile` (current behavior preserved,
   just switched to CMake).
7. Drop `src/include/base/config.h.in` (vestigial — not `#include`d).
8. Drop the orphan artpack `Makefile.am` files.
9. Add a CTest smoke test that runs `gltron -h`.

## Additional findings during decision review

- `src/audio/sound_stubs.c` and `src/audio/sound_glue_stubs.cpp` exist on
  disk but aren't linked. Per decision (2), these are dead and will be
  deleted along with the autotools files.
- `src/game/game_level.c` and `src/game/level.c` define the **same
  functions**. The autotools build only compiles `level.c`. CMake will
  match (don't compile `game_level.c`). `game_level.c` is dead code worth
  removing in a follow-up — flagged but not part of this PR.
- `src/game/32bit_warning.c` is on disk but not in any `Makefile.am`. Match
  autotools (skip it).
- `nebu/base/spline.c` is on disk but not built. Match (skip).
- `nebu/audio/music_rwop.c`, `nebu/video/{benchmark,extgl,light,
  png_texture,quad,quadbuf,renderer_gl,scene}.c`,
  `nebu/filesystem/{directory-macos,directory-win32,file-macos}.c`,
  `src/video/{hud,trails_buffered}.c` are on disk but not in any
  `Makefile.am`. Match (skip).
- `src/scripting/libscripting.a` is built but never linked. Match (skip).
- `lib3ds/file.c` and friends `#include <config.h>`, which resolves to the
  empty `lib3ds/config.h` because automake adds the source directory to
  the include path. CMake reproduces this with
  `target_include_directories(lib3ds PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})`.

## Goal

Replace the autotools build (`configure.ac` + 30 `Makefile.am` files) with a
single, modern CMake build (CMake ≥ 3.21). `Makefile.docker` stays GNU Make
and gains a target that uses the new CMake build to verify that gltron builds
reliably inside the Fedora container.

Non-goals:
- No code changes to gltron itself.
- No conversion to modern OpenGL.
- No new features. The CMake build must produce a binary that behaves
  identically to the autotools binary.

## Inventory of the current build

**Binary:** one — `gltron`, linked from `src/gltron.c` against the static
libs below. Final link uses the **C++ driver** (because `src/audio/` contains
`sound_glue.cpp`). `Makefile.am` does this with `gltron_LINK = $(CXX) -o $@`.

**In-tree static libs (linked into `gltron`, in this order):**

```
src/game/libgame.a
src/input/libinput.a
src/audio/libaudio.a              # contains C++
src/video/libvideo.a
src/configuration/libconfiguration.a
src/base/libbase.a
src/filesystem/libfilesystem.a
nebu/input/libinput.a
nebu/audio/libaudio.a             # contains C++
nebu/video/libvideo.a
nebu/scripting/libscripting.a
nebu/filesystem/libfilesystem.a
nebu/base/libbase.a
lua5/liblua.a                     # Lua 5.0, vendored
lua5/lib/liblualib.a              # Lua 5.0 stdlib, vendored
lib3ds/lib3ds.a                   # 3DS file format, vendored
```

`src/scripting/libscripting.a` exists in the source tree but is **not**
linked by the current build (the equivalent code lives in
`src/game/scripting_interface.c`). Worth confirming we leave it out.

**External dependencies:**

| Dep         | How found today               | Notes                                  |
|-------------|-------------------------------|----------------------------------------|
| SDL2        | `pkg-config sdl2 >= 2.0.0`    | required                               |
| OpenGL      | `-framework OpenGL` on macOS, `-lGL` elsewhere | required                |
| GLEW        | `AC_CHECK_LIB(GLEW, glewInit)`| required                               |
| libpng      | `AC_CHECK_LIB(png, ...)`      | required                               |
| zlib        | `AC_CHECK_LIB(z, gzopen)`     | required                               |
| SDL2_sound  | `pkg-config SDL2_sound`       | optional (`--disable-sound`); falls back to stubs (see *Stub mismatch* below) |
| SDL2_net    | `pkg-config SDL2_net`         | optional (`--enable-network`, off)     |

**Configure-time options (in `configure.ac`):**

- `--enable-warn` — `-Wall -Werror`
- `--enable-debug` — `-g3`
- `--enable-profile` — `-pg`
- `--enable-optimize[=N]` — `-O$N` (default `s`)
- `--disable-sound` — disable SDL2_sound
- `--enable-network` — pull in SDL2_net, define `NETWORK`
- `--enable-localdata` — define `LOCAL_DATA`; binary `chdir`s to its own
  directory and reads data relatively. Used by the Loki-style installer.
- `--with-snapshot-dir=PATH` — `-DSNAP_DIR=PATH` (default `~`)
- `--with-preferences-dir=PATH` — `-DPREF_DIR=PATH` (default `~`)

**Macros set at compile time (referenced in source):**

- `DATA_DIR`, `LOCAL_DATA`, `SNAP_DIR`, `PREF_DIR`, `RC_NAME`,
  `PATH_SEPARATOR`, `SEPARATOR`, `NETWORK`
- `RC_NAME` is hard-set to `".gltronrc"` for all builds.
- `PATH_SEPARATOR` and `SEPARATOR` are platform-specific (`/` on Unix,
  `\` on Windows).

**Header probes (`AC_CHECK_HEADERS`):** `unistd.h`, `GL/gl.h` /
`OpenGL/gl.h`. The probes write into `src/include/base/config.h` (generated
from `config.h.in`). That generated header records all the autoconf
`@prefix@`-style paths but **is not currently `#include`d anywhere in the
source tree** — confirmed by grep. So the generated file appears to be
vestigial.

**Function probes:** `strstr`, `mkstemp` — also unused in source.

**Installed data:** `art/`, `art/default/`, `data/`, `music/`, `scripts/`,
`sounds/`, `levels/`. Each is installed under `${pkgdatadir}` (i.e.
`${prefix}/share/gltron`). Other artpacks under `art/` (`arcade_spots`,
`biohazard`, `classic`, `metalTron`) have `Makefile.am` files but aren't
in the parent `art/Makefile.am` `SUBDIRS` — they're effectively dead in the
autotools build. We should match that for now.

**Stub mismatch (existing bug):** `configure.ac` substitutes `SOUND_OBJS`
between `sound.o sound_glue.o ...` and `sound_stubs.o sound_glue_stubs.o`
when SDL2_sound is missing — but `src/audio/Makefile.am` ignores
`SOUND_OBJS` entirely and always builds `sound.c sound_glue.cpp`, and there
are no `*_stubs.c`/`*_stubs.cpp` files in the tree. **`--disable-sound`
currently fails to build.** The CMake port should pick one behavior:
either (a) make sound mandatory and drop the option, or (b) actually create
working stubs. See open questions.

## Proposed CMake structure

```
CMakeLists.txt                # top-level: project, options, deps, install
cmake/
  GltronOptions.cmake         # options + cache vars (warn, debug, sound, …)
  GltronCompileDefs.cmake     # DATA_DIR, RC_NAME, PATH_SEPARATOR, …
lib3ds/CMakeLists.txt         # static lib (vendored)
lua5/CMakeLists.txt           # static libs liblua + liblualib
nebu/CMakeLists.txt           # adds subdirs
nebu/{base,filesystem,scripting,audio,video,input}/CMakeLists.txt
src/CMakeLists.txt            # adds subdirs + the gltron executable
src/{base,filesystem,configuration,audio,video,input,game}/CMakeLists.txt
```

Key choices:

- **Minimum:** `cmake_minimum_required(VERSION 3.21)`. 3.21 gives
  `OpenGL::OpenGL`, predictable target export, and is shipped on Fedora 43
  (the container) and recent Debian/Ubuntu.
- **Languages:** `project(gltron LANGUAGES C CXX)`. Final link via
  `set_target_properties(gltron PROPERTIES LINKER_LANGUAGE CXX)` to mirror
  `gltron_LINK = $(CXX)`.
- **C/C++ standards:** C99 (`set(CMAKE_C_STANDARD 99)`); the vendored Lua 5.0
  uses C89 — keep its old `-ansi -pedantic -Wall` via target-local flags.
  C++ left at the compiler default (gltron's C++ is minimal SDL_sound glue).
- **Static libs:** every component becomes an `add_library(<name> STATIC …)`
  target with a clear name (e.g. `nebu_base`, `gltron_game`, `lib3ds`,
  `lua`, `lualib`). Public include dirs declared with
  `target_include_directories(... PUBLIC …)` so dependents inherit them.
- **External deps:** `find_package(SDL2 REQUIRED)`, `find_package(OpenGL
  REQUIRED)`, `find_package(GLEW REQUIRED)`, `find_package(PNG REQUIRED)`,
  `find_package(ZLIB REQUIRED)`. SDL2_sound and SDL2_net via
  `pkg_check_modules` (no `find_package` modules ship for them).
- **Options (mirrors autotools, sensible defaults):**
  - `GLTRON_WARN` (default OFF) → `-Wall -Werror`
  - `CMAKE_BUILD_TYPE` (default `Release`) handles debug/release. The
    container can pass `-DCMAKE_BUILD_TYPE=Debug` to match today's
    `--enable-debug`.
  - `GLTRON_SOUND` (default ON) — see open question on stubs.
  - `GLTRON_NETWORK` (default OFF) → SDL2_net + `-DNETWORK`.
  - `GLTRON_LOCALDATA` (default OFF) → `-DLOCAL_DATA`.
  - `GLTRON_SNAPSHOT_DIR` (default `~`) → `-DSNAP_DIR=…`.
  - `GLTRON_PREFERENCES_DIR` (default `~`) → `-DPREF_DIR=…`.
  - Drop `--enable-profile`; users can pass `-pg` via `CFLAGS` if they want.
- **Compile defs** applied as `target_compile_definitions(gltron_filesystem …)`
  etc., same scope as today (per-library, since `path.c` and friends are
  the consumers).
- **Install:** `install(TARGETS gltron RUNTIME DESTINATION bin)` and
  `install(DIRECTORY data/ art/default/ music/ scripts/ sounds/ levels/
  DESTINATION share/gltron/…)` mirroring the current layout. `DATA_DIR`
  set to `${CMAKE_INSTALL_FULL_DATADIR}/gltron` via
  `GNUInstallDirs`.
- **`compile_commands.json`:** auto-emitted; `bear` no longer needed in the
  container (we can leave it installed but stop using it).
- **Out-of-tree only:** add a guard that errors if `CMAKE_BINARY_DIR ==
  CMAKE_SOURCE_DIR`.

## Verification strategy

Inside the container:

1. `cmake -S /gltron -B /bld -G Ninja -DCMAKE_BUILD_TYPE=Debug`
2. `cmake --build /bld -j`
3. Inspect: `/bld/gltron --help` (or `-h`) exits 0.
4. Optional: a CTest smoke test that runs `gltron -h` with
   `SDL_VIDEODRIVER=dummy`.
5. A second build with `-DGLTRON_NETWORK=ON` (if SDL2_net is in the
   container) to confirm the option still wires up.

Bill verifies the actual gameplay binary on his Linux desktop.

## `Makefile.docker` changes

Add a `build` target that does the configure + build inside an ephemeral
container, mounting the repo. Sketch:

```make
.PHONY: build
build: image ## CMake configure + build inside the container, into ./bld
	$(CONTAINER_CMD) run --rm \
		$(FILES_TO_MOUNT) \
		$(CONTAINER_NAME) \
		bash -lc 'cmake -S /gltron -B /gltron/bld -G Ninja \
		         -DCMAKE_BUILD_TYPE=Debug && cmake --build /gltron/bld -j'
```

Open: should the `Dockerfile` still build at image-build time, or rely on
the `build` target post-image-build? See open questions.

`Makefile.docker` itself stays in GNU Make — the only changes are adding
the `build` (and possibly `test`) target(s) and dropping `bear` from the
shell entrypoint if it's wired in there.

## What landed (2026-04-28)

- `CMakeLists.txt` at the top level + 14 sub-`CMakeLists.txt` files mirroring
  the old `Makefile.am` leaves: `lib3ds/`, `lua5/`, `nebu/` and its six
  module dirs, `src/` and its seven module dirs.
- Two interface targets carry cross-cutting state:
  - `gltron_paths` — the `DATA_DIR`, `RC_NAME`, `SNAP_DIR`, `PREF_DIR`,
    `PATH_SEPARATOR`, `SEPARATOR`, `VERSION` defines that the autotools
    build sprayed via global `CFLAGS`.
  - `gltron_warnings` — `-Wall -Werror` when `-DGLTRON_WARN=ON`, applied to
    first-party libs only (vendored `lib3ds` / `lua5` are exempt because
    they don't pass `-Werror`).
- `nebu_includes` is the public include hub for all of nebu. It propagates
  Lua and SDL2 transitively because nebu's own public headers `#include`
  `"lua.h"` and `"SDL.h"`.
- The final link is forced to the C++ driver via `LINKER_LANGUAGE CXX` to
  match `gltron_LINK = $(CXX)` from the old top-level `Makefile.am`
  (needed for `src/audio/sound_glue.cpp` and `nebu/audio/*.cpp`).
- `CMAKE_C_EXTENSIONS ON` (the default `gnu99` rather than strict `c99`);
  `nebu/filesystem/findpath.c` calls `realpath()`, which the strict ISO
  mode hides behind feature-test macros. Autotools didn't pass `-std`, so
  this matches its behavior.
- `GLTRON_NETWORK` is the only carry-over option; SDL2_net is only required
  when it's on.
- CTest has one trivial test that confirms the binary exists, since the
  game `assert(argc == 1)`s on any CLI flag and otherwise needs a display.
- Top-level guard against in-tree builds.
- Install layout (`${prefix}/bin/gltron`, `${prefix}/share/gltron/{art,
  data,music,scripts,sounds,levels}`) verified by `cmake --install` to a
  staging dir — matches what `make install` produced before.

## Files removed

- `configure.ac`, `autogen.sh`
- All `Makefile.am` (50 files)
- `src/include/base/config.h.in` (was generated but never `#include`d)
- `src/audio/sound_stubs.c`, `src/audio/sound_glue_stubs.cpp` (dead — the
  autotools `--disable-sound` path that referenced them never built)
- `src/scripting/` (empty directory after Makefile.am removal; its
  `Makefile.am` referenced `scripting_interface.c`, which actually lives
  in `src/game/`, so the lib it built was never linked anywhere)

## Container changes

- `Dockerfile`: dropped `autoconf automake libtool bear`; added `cmake
  ninja-build SDL2-devel`. The `RUN` stanza now invokes `cmake -G Ninja`
  + `cmake --build` + `cmake --install` + `ctest`. Image still ships a
  ready-to-run binary at `/bldInstall/bin/gltron`.
- `Makefile.docker`: new `build` target re-runs the CMake configure and
  build against the mounted source tree, writing to `./bld/`. Used for
  iterating without rebuilding the image.
- `INSTALL`, `CLAUDE.md` build sections rewritten.

## Notes / follow-ups

- **`src/game/game_level.c` is dead code** — defines the same functions as
  `level.c`. Autotools never compiled it, the new CMake never compiles it,
  but the file still sits in the tree. Worth deleting in a follow-up.
- **`src/game/init.c:56` does `assert(argc == 1)`** — gltron rejects all
  CLI args. The README's claim about `-h` and `-O` is stale. If we ever
  want `gltron -h` to print usage, that's a separate (small) source
  change, not a build-system one.
- **Other on-disk-but-unbuilt sources** worth a sweep someday:
  `src/game/32bit_warning.c`, `nebu/base/spline.c`,
  `nebu/audio/music_rwop.c`, several `nebu/video/*.c` files,
  `nebu/filesystem/{directory-macos,directory-win32,file-macos}.c`,
  `src/video/{hud,trails_buffered}.c`. Some might be worth resurrecting
  (the `nebu/filesystem` per-platform files in particular).
- **Orphaned artpacks** `art/{arcade_spots,biohazard,classic,metalTron}/`
  still ship their assets; only their `Makefile.am` files were deleted.
  Wiring them into the install step would be a small follow-up.
