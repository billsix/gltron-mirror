# Re-think how the SDL2 window is plumbed through the codebase

**Status:** not started — Bill flagged it 2026-04-28 and wants to discuss
the approach before any code moves.

## Background

When the autotools→SDL2 port happened, the singleton-window assumption from
SDL 1.2 was carried forward. Quick grep:

- `nebu/video/video_system.c:11` — `static SDL_Window* gWindow = NULL;`
- `nebu/base/system.c:104` — `SDL_Window* current_win = SDL_GL_GetCurrentWindow();`
- `nebu/input/input_system.c:65,72` — same `SDL_GL_GetCurrentWindow()` lookup.

So today there are two coexisting access patterns: one file-scope global,
plus on-demand `SDL_GL_GetCurrentWindow()` calls elsewhere. SDL2 (and SDL3)
both support multiple windows, so the singleton assumption is structural
debt rather than an SDL constraint.

## What this plan should produce

1. Audit every reference to the window (and the GL context, since they
   travel together). Confirm whether anything actually creates / would
   want more than one window — fullscreen toggle, screenshot helper,
   demo recording, etc.
2. Compare two structural options:
   - (a) Keep a single window but route it through an explicit
     "video subsystem" struct that's threaded through the engine
     (no global, no `SDL_GL_GetCurrentWindow` lookups).
   - (b) Generalize to N windows (probably overkill — gltron's a single
     viewport game).
3. Recommend one, with the reasoning, and only then ask Bill whether
   to implement it. The bias should be the smallest change that removes
   the hidden global, not a renderer rewrite.

## Open questions for Bill

- Is there an actual planned use for multiple windows (e.g. picture-in-
  picture mini-map, level editor preview), or is this purely about
  removing the global?
- Should the cleanup be done as one PR, or as part of the SDL3 port
  (next plan)?

## Ordering

Should land before the SDL3 port. Touching the window plumbing twice
(once for cleanup, once for the SDL3 API rename) is more churn than doing
it once during the SDL3 work — but Bill has signalled he wants the design
discussion separately, so plan it as its own step.
