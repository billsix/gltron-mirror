# AppImage build target in Makefile.docker

**Status:** Shipped 2026-04-28 as a *host-dependent* AppImage (only runs on
Fedora 43-class hosts that already have SDL2/SDL2_sound/libpng/GLEW installed).
The "Future work" section below describes what's needed to make it fully
self-contained.

## What ships today

`make -f Makefile.docker appimage` produces
`out/gltron-<version>-Linux.appimage` by:

1. Building the podman image (Fedora 43 + AppImage tooling).
2. Inside an ephemeral container, running a clean Release CMake build with
   `-DGLTRON_DATA_DIR=usr/share/gltron` so the binary's compile-time
   `DATA_DIR` is a relative path that resolves against the AppDir root at
   runtime.
3. Running `cpack -G External` against that build.
4. Copying the resulting `.appimage` to `./out/` on the host.

`cpack -G External` runs a generated CMake script
(`bld-appimage/appimage-generate.cmake`) that:

1. Downloads `appimagetool` (continuous build) on first run, caching it
   under the build dir.
2. Stages `AppRun`, `gltron.desktop`, and `gltron.png` at the AppDir
   root (alongside the cpack-installed `usr/` tree, which is what
   `CPACK_TEMPORARY_DIRECTORY` points at).
3. Invokes `appimagetool --appimage-extract-and-run` (no FUSE needed) to
   package the AppDir.

### File inventory

- `CMakeLists.txt` — drops `FORCE` from `GLTRON_DATA_DIR` so `-D` overrides
  work; sets `APPIMAGE_DESKTOP_FILE` / `APPIMAGE_ICON_FILE` /
  `APPIMAGE_APPRUN` properties on the `gltron` target; selects
  `CPACK_GENERATOR=External` on Linux; includes `CMake/Packaging.cmake`.
- `CMake/Packaging.cmake` — CPack config + `file(GENERATE)`-emitted
  `appimage-generate.cmake` that drives `appimagetool`.
- `CMake/Packaging-config.cmake` — sets
  `CPACK_PACKAGING_INSTALL_PREFIX=/usr` and `CPACK_MONOLITHIC_INSTALL=1`
  when the External generator is active.
- `packaging/AppRun` — POSIX shell launcher that `cd`s into `$APPDIR`
  (or the script's own directory if launched from an extracted AppDir)
  and execs `./usr/bin/gltron`. Needed because the relative `DATA_DIR`
  resolves against cwd, and linuxdeploy's default is a *symlink* AppRun
  that doesn't change cwd.
- `packaging/gltron.desktop` — minimal freedesktop entry.
- `Dockerfile` — adds `desktop-file-utils`, `file`, `fuse-libs`, `patchelf`,
  `squashfs-tools`, `wget` to the dnf install (some are leftovers from the
  linuxdeploy attempt — see "Cleanup ideas" below). Also `COPY`s `CMake/`
  and `packaging/` into the image so the in-image build sees them.
- `Makefile.docker` — `appimage` target. `--entrypoint /bin/bash` is
  required because the image's `ENTRYPOINT ["/entrypoint.sh"]` references
  a file that isn't COPY'd in (same issue affects the existing `build`
  target, separate problem).

### What the AppImage actually contains

```
AppRun                      (shell script)
gltron.desktop              (root copy for AppImage runtime)
gltron.png                  (root copy for AppImage runtime)
usr/bin/gltron
usr/share/applications/gltron.desktop
usr/share/icons/hicolor/256x256/apps/gltron.png
usr/share/gltron/{art,data,levels,music,scripts,sounds}/...
```

No bundled libraries. The binary's `libSDL2-2.0.so.0`,
`libSDL2_sound.so.2`, `libpng16.so.16`, `libGLEW.so.2.2` etc. resolve
against the host's `/lib64` at runtime.

## Dead ends — do not retry without new information

We tried `linuxdeploy` (the canonical CMake-friendly AppImage bundler, and
that was the original plan here). Both the pinned `1-alpha-20240109-1`
release and the much newer `continuous` build
produced AppImages where every bundled lib (libSDL2, libSDL2_sound,
libpng16, in that order as I removed them) crashed at the **same offset
0x2cc** in `dl_init` on Bill's host. Pattern:

```
Program received signal SIGSEGV.
0x00007ffff7d2a2cc in ??  (← inside bundled libSDL2-2.0.so.0 at offset 0x2cc)
#1  call_init at dl-init.c:60
#2  _dl_init at dl-init.c:121
```

After `mv squashfs-root/usr/lib/libSDL2-2.0.so.0` out of the way, the
crash repeated at `0x2cc` in the *next* bundled lib (libSDL2_sound), then
libpng16 once that was moved. Same offset across three different libs is
not coincidence — something about the bundling itself corrupts F43 libs in
a way that survives every linuxdeploy version we tried. Suspected (but
not confirmed) cause: linuxdeploy's bundled `patchelf` mishandles the
`.relr.dyn` section (DT_RELR packed relative relocations, glibc 2.36+).
The matching `strip` failure (`unknown type [0x13] section '.relr.dyn'`)
that we worked around with `NO_STRIP=1` is suggestive but separate.

If a future session wants to reopen this, **don't just upgrade
linuxdeploy** — that didn't help. Either build deps from source so the
bundled libs aren't F43's, or replace the bundling step entirely (see
"Future work").

## Future work — make the AppImage self-contained

Goal: an AppImage that runs on any Linux host with glibc ≥ 2.42, not just
Fedora 43-class systems. Required because right now if Bill hands the
`.appimage` to anyone on Ubuntu, Debian, Arch, etc., it'll fail to find
`libSDL2-2.0.so.0` (or find an incompatible one).

### Approach: build SDL2 + SDL2_sound from source in the Dockerfile

Clone SDL on the SDL2 branch, cmake-build, install into the container.
This avoids the SDL2-compat shim entirely and gives us a clean known-good
`libSDL2` that's safe to bundle.

Steps:

1. **Add to `Dockerfile`** (after the dnf install, before the COPY block):
   ```dockerfile
   # Real SDL2 from source — overwrites the SDL2-compat shim that F43 ships.
   # Necessary because the shim dlopens SDL3 at runtime, and bundling that
   # behavior into an AppImage that runs on a different host is fragile.
   RUN git clone --depth 1 -b SDL2 https://github.com/libsdl-org/SDL.git /tmp/SDL \
    && cmake -S /tmp/SDL -B /tmp/SDL/bld -G Ninja -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib64 \
    && cmake --build /tmp/SDL/bld -j \
    && cmake --install /tmp/SDL/bld \
    && rm -rf /tmp/SDL && ldconfig

   # SDL_sound source build, against our just-installed real SDL2.
   RUN git clone --depth 1 https://github.com/icculus/SDL_sound.git /tmp/SDL_sound \
    && cmake -S /tmp/SDL_sound -B /tmp/SDL_sound/bld -G Ninja \
             -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib64 \
    && cmake --build /tmp/SDL_sound/bld -j \
    && cmake --install /tmp/SDL_sound/bld \
    && rm -rf /tmp/SDL_sound && ldconfig
   ```
   Add `git` to the dnf install list. Keep the existing `SDL2`/`SDL2-devel`
   dnf packages installed — they pull in the X11 / audio devel headers
   that SDL2's source build needs. Installing the source SDL2 to `/usr`
   (not `/usr/local`) overwrites the compat shim, so there's exactly one
   `libSDL2-2.0.so.0` on the system afterwards and pkg-config / linker
   pick it up automatically.

2. **Re-introduce dependency bundling in `CMake/Packaging.cmake`.** With
   real SDL2 in the build container, bundling is safe again. Two ways:

   - (a) Switch back to linuxdeploy, but only after verifying it doesn't
     hit the `0x2cc` corruption on source-built libs. The corruption may
     have been specific to F43's pre-linked DT_RELR-using shim.
   - (b) Keep the appimagetool-direct approach we have today and copy the
     deps in by hand. Walk `ldd $<TARGET_FILE:gltron>` (excluding the
     standard "blacklist": libc, libm, libGL/libGLX/libOpenGL, libstdc++,
     libgcc_s, libX11, libxcb), copy each into `${APPDIR}/usr/lib/`, and
     `patchelf --set-rpath '$ORIGIN'` them. Then `patchelf --set-rpath
     '$ORIGIN/../lib'` the binary. Roughly 30 lines of CMake.

   Option (b) is more work but avoids re-litigating the linuxdeploy
   corruption question. Option (a) is the canonical path if it works.

3. **Verify on a non-F43 host.** Spin up an Ubuntu LTS VM (or just docker
   run an Ubuntu image with `--device /dev/dri`), copy the `.appimage`
   over, and confirm it launches. Without this check, "self-contained"
   is just an assumption.

### Smaller cleanup that can happen any time

- Drop unused dnf packages from `Dockerfile`: `wget` (we use cmake's
  `file(DOWNLOAD)` instead), `patchelf` (no longer modifying ELFs),
  `fuse-libs` (appimagetool runs with `--appimage-extract-and-run`).
  Keep `desktop-file-utils`, `file`, `squashfs-tools` — appimagetool
  needs them at AppImage-creation time.
- Fix the broken `make -f Makefile.docker build` target the same way
  `appimage` was fixed: add `--entrypoint /bin/bash` so it doesn't try
  to invoke the missing `/entrypoint.sh`. Out of scope for the AppImage
  work but trivially adjacent.

## Notes for whoever picks this up

- `GLTRON_DATA_DIR=usr/share/gltron` is *relative* on purpose. The custom
  `packaging/AppRun` `cd`s into `$APPDIR` before exec'ing the binary, so
  relative paths resolve against the AppDir root. Don't change DATA_DIR
  to absolute without also rewriting AppRun to keep the binary's view
  consistent.
- linuxdeploy makes its default AppRun a *symlink* directly to the
  binary, which means the binary's cwd is whatever the user launched
  from — that's why we need our own AppRun.
- `--appimage-extract-and-run` everywhere avoids needing FUSE inside the
  build container and on hosts that don't have FUSE configured.
