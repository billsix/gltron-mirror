# Modernize display handling: fullscreen, resolutions, arbitrary aspect ratio

**Status:** proposed — not started
**Priority:** 5
**Difficulty:** 7
**Created:** 2026-06-13

## Goal

Bring gltron's main-screen / display handling up to modern standards: a clean
fullscreen toggle, runtime resolution selection, and — the headline requirement —
**look good at an arbitrary aspect ratio**, not just 4:3/16:9. On an ultrawide or
an unusual window size the 3D scene and the HUD/menus should adapt gracefully
(no stretching, squashing, or mis-placed UI), the way a modern game does.

Builds on the already-shipped single-window refactor
(`tasks/archive/2026/05/10/sdl-window-architecture.md`) and the SDL3 port
(`tasks/sdl3-port.md`). Fixed-function OpenGL stays — adapting the viewport
and projection *is* part of the graphics lesson here; keep the surrounding style.

## Plan

- [ ] **Inventory current display handling.** How the window/context is created
      (SDL3, single window via `nebu_Video_GetWindow()` accessors), where
      resolution/fullscreen live in `src/configuration/`, and the projection +
      viewport setup in `src/video/` and `nebu/video/` (`gluPerspective`/`glFrustum`,
      `glViewport`, the 2D ortho used for HUD/console/menus, font scaling). Find
      every place an aspect ratio or fixed resolution is *assumed*.
- [ ] **Catalog what breaks off-ratio.** Reproduce at e.g. 21:9 and a tall window:
      stretched HUD, wrong field-of-view, GUI elements placed by hardcoded pixel
      coords, menu/console layout, fonts. List the concrete offenders.
- [ ] **Resolution & fullscreen.** Enumerate display modes
      (`SDL_GetFullscreenDisplayModes`), support borderless fullscreen-desktop vs.
      exclusive, runtime switching, and — critically — handle
      `SDL_EVENT_WINDOW_RESIZED` / `PIXEL_SIZE_CHANGED` to recompute viewport +
      projection on the fly.
- [ ] **Pick an aspect-ratio strategy for the 3D scene.** Recommend **Hor+** (hold
      vertical FOV constant, widen horizontal FOV by the aspect ratio) so wider
      windows reveal more world horizontally instead of stretching — standard for
      this kind of game. Document the alternatives (Vert-, pillarbox/letterbox)
      and why.
- [ ] **Make the 2D HUD/menus resolution-independent.** Replace hardcoded pixel
      coords with an anchor-based layout over an ortho projection that adapts to
      the window; scale fonts/console by window height (or DPI). Confirm against
      gltron's GUI/console system (which is partly Lua-scripted).
- [ ] **Config + UI.** Persist resolution / fullscreen / chosen aspect behavior in
      `~/.gltronrc`, and surface them in the in-game settings menu.
- [ ] **Write the plan.** Produce `tasks/display-modes-aspect-ratio.md` (per
      gltron's convention) with the chosen strategy, the offender list, a phased
      approach, and risks; add it to the `## Tasks / plans (in-flight)` index in
      `CLAUDE.md`.

## Notes / decisions

- The single-window accessor work and SDL3 port are already done — this sits on
  top of them; the window plumbing exists, the *adaptation logic* is the gap.
- Fixed-function pipeline is preserved on purpose; viewport/projection changes are
  legitimate graphics teaching content, but match the existing code style and keep
  edits surgical (no renderer-abstraction rewrites).
- This `tasks/` item tracks the investigation; the durable plan goes in `tasks/`.

## Open questions

- Aspect strategy for the 3D view: **Hor+** (recommended), Vert-, or pillarbox?
- Default fullscreen mode: borderless fullscreen-desktop (safer, modern) vs.
  exclusive (real mode switch)?
- How resolution-independent is the current GUI/console/menu system, and is the
  Lua-scripted layer easy to drive from a design-resolution scheme or does it need
  rework?
