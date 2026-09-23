# Install the image-built gltron into /usr/local (bin on PATH), not /bldInstall

**Status:** proposed — needs go-ahead (William Emerison Six <billsix@gmail.com>, 2026-09-23,
while giving smc the same layout: "I want the same thing for smc, but in /usr/local/bin,
actually, make a task in gltron to do the same if it installs somewhere else")
**Priority:** 5 · **Difficulty:** 2 (one `CMAKE_INSTALL_PREFIX`, one `run` path, the docs)
**Started:** —

## BLUF

The `Dockerfile` builds gltron at image-build time but installs it to `/bldInstall`
(`-DCMAKE_INSTALL_PREFIX=/bldInstall`, `Dockerfile:44-46`), so the binary is not on `PATH` and the
image's `run`/`shell` paths have to know that directory. Change the image build to install into
`/usr/local` (`/usr/local/bin/gltron`, data under `/usr/local/share/gltron`), the standard prefix
smc now uses (github.com/billsix/smc — see its `entrypoint/build.sh`), and update every path
that pointed at `/bldInstall`. Done = `make image` green, `podman run … gltron` finds `gltron` on
`PATH`, `make run`/`make shell` unchanged for the user, docs updated.

## Context

- `Dockerfile:44-49`: `cmake -S /gltron -B /bld … -DCMAKE_INSTALL_PREFIX=/bldInstall && cmake
  --build && cmake --install /bld && ctest`. `Makefile.docker:41-46` (the `image`-time rebuild
  from the mounted tree) installs to `/gltron/bld/install` — a second, different prefix.
- `CMakeLists.txt:32` derives `GLTRON_DATA_DIR` from `CMAKE_INSTALL_FULL_DATADIR`, so a
  `/usr/local` prefix puts data at `/usr/local/share/gltron` with no other change.
- Grep for the old prefix before finishing: `rg -n 'bldInstall|bld/install' Dockerfile
  Makefile.docker entrypoint README CLAUDE.md tasks/reference` — every hit is a path to update
  (the AppImage target's `-DGLTRON_DATA_DIR=usr/share/gltron` at `Makefile.docker:70-72` is a
  different, relative layout and stays).
- Why: one conventional prefix, the binary on `PATH` in any shell in the image, and the
  bind-mounted `make shell` no longer needs a host-side install directory. smc's `build.sh`
  (2026-09-23) is the worked example of the same change: CEGUI and the game both into
  `/usr/local`, an `ld.so.conf.d` entry + `ldconfig` for the private shared libraries.

## Plan

- [ ] `Dockerfile`: `-DCMAKE_INSTALL_PREFIX=/usr/local`; keep `/bld` as the build tree.
- [ ] `Makefile.docker` `image`-time rebuild and any `run`/`shell` path: same prefix, or run
      `gltron` from `PATH`.
- [ ] Docs: `README`/`CLAUDE.md`/reference docs that name `/bldInstall`.
- [ ] Verify nested: `make image` green, `podman run --rm --entrypoint /bin/bash gltron -c
      'command -v gltron && ls /usr/local/share/gltron'`.

## Notes / decisions

## Open questions

None.
