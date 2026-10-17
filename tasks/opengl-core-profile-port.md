# Port from fixed-function OpenGL to OpenGL 3.3 core profile

**Status:** not started — Bill flagged it 2026-04-28. Independent of the
SDL plans (could be tackled in any order), but big enough to discuss
scope with him before starting.
**Priority:** 8
**Difficulty:** 9

## Goal

Replace every fixed-function GL call with shaders and VAO/VBO-based
rendering, **keeping the visible output identical**. The shaders should
do exactly what the FFP did — no new effects, no quality changes, no
"while we're in there" cleanups.

The teaching value of the port is precisely the side-by-side: "here's
what FFP does, and here's the GLSL/CPU code that produces the same
pixels in core profile." Every drift from that defeats the lesson.

## Related cleanup (optional; from the C modernization sweep 2026-10-17)

The C modernization sweep (`tasks/archive/2026/10/17/c-modernization-sweep.md`) left a few
**GL-id-holding fields/params typed as `int`**, casting `(GLuint)`/`(GLuint*)` locally at the GL
call sites rather than retyping across a header (its no-cross-dir-header rule). Retyping these to
the real GL types would drop ~8 local casts and is natural to fold into this port, since you will be
rewriting that GL code anyway:

- `video_level_shader.diffuse_texture_id` (`int` in `src/include/video/video_level.h`)
- `nebu_2d::tex_id` (`int` in `nebu/include/.../nebu_2d.h`)
- `nebu_Font::pTextures` (`int*` in `nebu/include/.../nebu_font.h`)
- `trailStatesNormal(Player*, int texture)` param (prototype in `video/trail_geometry.h`)

Not a prerequisite for the port; just the cheapest moment to do it. Keep behavior identical.

## What this plan should produce

1. Inventory FFP usage. Surface map by category:
   - Matrix stack (`glMatrixMode`, `glLoadIdentity`, `glPushMatrix`,
     `glLoadMatrixf`, `glMultMatrixf`, `glTranslatef`, `glRotatef`,
     `glScalef`, `glOrtho`, `glFrustum`).
   - Immediate mode (`glBegin`/`glEnd`, `glVertex*`, `glColor*`,
     `glNormal*`, `glTexCoord*`).
   - Lighting (`glLightfv`, `glLightModel*`, `glMaterial*`,
     `glColorMaterial`).
   - Vertex arrays in compatibility form (`glEnableClientState`,
     `glVertexPointer`, `glColorPointer`, `glNormalPointer`,
     `glTexCoordPointer`).
   - Texture environment (`glTexEnvi`, `glAlphaFunc`).
   - Misc (`glPolygonMode`, `glShadeModel`).
2. Pick the smallest replacement primitives for each:
   - One CPU-side matrix library (we already have `nebu/base/matrix.c` —
     reuse it for everything; no glm).
   - One streaming VBO/VAO for immediate-mode replacement (write into
     a CPU buffer, glBufferSubData, draw). One-shot static VBOs for
     fixed geometry.
   - One Blinn-Phong-ish vertex+fragment shader pair that mirrors FFP's
     per-vertex Phong with optional texturing — that's the FFP default
     behavior. Single shader pair if possible; specialize via #defines
     or uniforms only if necessary.
3. Stage the work so the codebase always builds and runs. Suggested
   stages:
   - (a) Add a small "draw queue" abstraction in nebu that hides FFP
     calls behind functions (e.g. `nebu_draw_quads_2d`,
     `nebu_set_camera`). Switch all call sites to the abstraction
     while still calling FFP under the hood.
   - (b) Rewrite the abstraction's implementation in core profile.
     Now no game code changes; only nebu's renderer flips.
   - (c) Switch the GL context request from compatibility to 3.3 core.
4. Decide on the GL context-request location. Probably
   `nebu/video/video_system.c` where the SDL window is created; the
   `SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, ...)` pair plus
   version request goes in the same place.

## Open questions for Bill

- Is the fixed-function pipeline still pedagogically central, or has
  this become a "modernize the codebase" goal? That changes whether
  we keep the FFP path alive behind a build flag (so demos can show
  both) or just rip it out.
- Lighting: gltron uses GL_LIGHT0 with mostly-default material. Do
  you want the shaders to match FFP's lighting model exactly
  (per-vertex Phong with the OpenGL fixed equations) or are
  approximate-equivalent shaders fine?
- Do we keep the `glBegin/glEnd` style in the heads-up-display 2D code
  via a tiny "immediate-mode emulation" helper, or rewrite the HUD
  code to push vertices into a buffer? The former matches the
  teaching aesthetic; the latter is closer to how modern code is
  actually written.

## Ordering

Independent of the SDL plans. If any single plan opens the door for
the others, the SDL window-architecture cleanup is a natural prerequisite
to *both* SDL3 and the GL port (touching the GL context creation site
once is cheaper than three times). Suggest order:

1. Window architecture cleanup
2. SDL3 port (or GL core port — either order works after #1)
3. Whichever wasn't done in #2
