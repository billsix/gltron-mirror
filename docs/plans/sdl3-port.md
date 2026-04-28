# Port from SDL2 to SDL3

**Status:** not started — gated on the
[SDL window architecture](sdl-window-architecture.md) cleanup. Don't begin
without confirming with Bill.

## Why now

- Fedora 43's `libSDL2-2.0.so.0` is the SDL2-compat shim on top of SDL3.
  The shim is fine for casual use but introduces hazards (see
  [appimage-target.md](appimage-target.md) — bundling F43's SDL2 libs
  into an AppImage produced consistent `dl_init` segfaults, and the
  recommended workaround on the user-memory side is to bundle real SDL2).
- SDL2 is in maintenance mode upstream. SDL3 is the supported line.
- Porting now (rather than later) keeps the codebase usable as a teaching
  artifact without an extra layer of compat-shim explanation.

## What this plan should produce

1. Read the SDL3 migration guide. Inventory every SDL2 API gltron and
   nebu touch — there's a focused surface (window, GL context, events,
   timer, audio init, joystick, keyname lookups, mutex/semaphore).
2. Decide on the main-loop model. SDL3 prefers the
   `SDL_AppInit/SDL_AppIterate/SDL_AppEvent/SDL_AppQuit` callback shape;
   gltron currently has its own `while(running)` loop. The classic loop
   still works in SDL3, so probably keep it for clarity (this is a
   teaching codebase) unless there's a concrete reason to switch.
3. Identify every `SDL_*` symbol that was renamed/removed (e.g.
   `SDL_GetTicks` returns `Uint64` now, `SDL_RWFromFile` →
   `SDL_IOFromFile`, etc.) and produce a focused diff.
4. Audio: gltron uses SDL2_sound. SDL3_sound exists; check whether our
   call sites need changes.
5. SDL2_net (currently optional via `GLTRON_NETWORK`): SDL3_net exists;
   minor renames expected.

## Pre-flight: dependencies on the previous plan

If we keep the global `gWindow`, the SDL3 port can be a near-mechanical
rename. If we restructure to a video-subsystem struct first, the SDL3
port becomes "rename APIs and update the struct's fields". Either order
works mechanically — Bill's preference was to settle the architecture
question first.

## Container/build implications

- The dev-container Dockerfile currently installs `SDL2-devel` and
  `SDL2_sound-devel` from dnf. Post-port, swap to `SDL3-devel` (or
  build SDL3 from source if F43 doesn't ship it directly). Same for
  SDL3_sound.
- The host SDL2 is the SDL2-compat shim *over* SDL3, so SDL3 is
  already installed on Bill's host — link will work without bundling.
- AppImage: a real SDL3-linked binary on Bill's F43 host should work
  with no library bundling at all (host has SDL3). For portable
  AppImages, refer to the future-work section of the appimage plan.
