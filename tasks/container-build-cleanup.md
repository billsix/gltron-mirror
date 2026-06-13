# Clean up the podman container build (Dockerfile / Makefile.docker / entrypoint)

**Status:** proposed — not started
**Created:** 2026-06-13

## Goal

Fix vestigial copy-paste and drift in gltron's container plumbing
(`Dockerfile`, `Makefile.docker`, `entrypoint/`). These are container papercuts
only — none touch the game, the CMake build, or the asset layout. The
`Makefile.docker` is otherwise clean and conformant with the family
"Fedora-44 + podman ephemeral-container" template; the `Makefile.docker` name
(invoked `make -f Makefile.docker <target>`) is a deliberate divergence because
gltron's own build is CMake.

## Plan

- [ ] **Dead `ENTRYPOINT` + stale script (highest impact).** `Dockerfile:107`
      declares `ENTRYPOINT ["/entrypoint.sh"]`, but nothing is `COPY`'d from
      `entrypoint/` into the image (`shell.sh`/`format.sh` arrive via runtime
      bind-mounts; `entrypoint.sh` isn't copied at all), so `/entrypoint.sh`
      doesn't exist in the image. `entrypoint/entrypoint.sh` is still
      programmingFromTheGroundUp's pgu-docs script verbatim (`cd /pgu/docs` →
      `/output/pgu/`). `shell`/`format`/`appimage` pass `--entrypoint /bin/bash`
      so they're fine, but the `build` target does *not* override the
      entrypoint (`… $(CONTAINER_NAME) bash -lc '…'` → `/entrypoint.sh bash -lc`
      → fails at runtime); bare `podman run gltron` fails too.
      **Fix (apue precedent):** delete `entrypoint/entrypoint.sh` and remove the
      `ENTRYPOINT` line. With no entrypoint, `build`'s `bash -lc '…'` runs as the
      command directly — also un-breaks `build`.
- [ ] **Undefined build-arg.** `image:` passes `--build-arg BUILD_DOCS=$(BUILD_DOCS)`,
      but `BUILD_DOCS` is defined nowhere (only `USE_GRAPHICS ?= 1`) and the
      Dockerfile has no such `ARG` → expands to empty, podman warns "unconsumed."
      **Fix:** drop that `--build-arg BUILD_DOCS=...` line (gltron builds no docs).
- [ ] **Stale `format.sh` paths.** Runs `find lib3ds/ nebutest/ nebu src/ …`,
      but `lib3ds/` and `nebutest/` no longer exist → two `find: No such file or
      directory` errors per run. **Fix:** `find nebu/ src/ …`.
- [ ] *(optional)* No `lint.sh` despite a `.clang-tidy` present. The C-family
      template usually pairs `format.sh` with a `lint.sh` (`run-clang-tidy`) and
      a `lint` target. Wire one up if we want clang-tidy in the container flow,
      like spimulator/texExpToPng.

## Notes / decisions

- Verified on commit `d3716151` ("maybe q button fix").
- Container targets are invoked via `make -f Makefile.docker <target>`, not a
  bare `make` (the bare `Makefile` name is reserved away from CMake).

## Open questions

- None blocking. Decide whether the optional `lint.sh` is in scope.

## Out of scope

- No changes to the game, the CMake build, or assets. No drive-by
  "modernization" — per repo conventions the codebase is a teaching artifact.
