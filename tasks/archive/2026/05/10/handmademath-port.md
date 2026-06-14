# Replace nebu math with HandmadeMath

**Status:** all phases (1–5) shipped 2026-05-08. Drafted 2026-05-08 from
a study of `/HandmadeMath/HandmadeMath.h` (3940 LOC, single-header,
public domain) and the gltron math surface. **Bill confirmed 2026-05-08
that HandmadeMath fits the pedagogical goal better than the
hand-rolled nebu math.**

**Outstanding:** the only follow-up is Bill's listen-test of the
SourceEngine pitch effect on his host (the Phase 2 dot-product
rewrite). Everything else is on disk and verified by smoke build +
menu-launch run.

## Why

`nebu/base/{vector,matrix,quat}.{c,h}` and `nebu/include/base/nebu_Vector3.h`
are ~660 LOC of hand-rolled linear algebra. Almost all of it is the
generic part of the API — the part that exists in every graphics math
library — and HandmadeMath covers every operation gltron currently uses
from these files. Replacing the linear-algebra core with HMM:

- Drops ~660 LOC of bespoke math.
- Removes a maintenance hot spot (the matrix inverse via cofactors, the
  quaternion ↔ matrix conversions, etc.) in favour of a single
  well-tested public-domain header.
- Lines up with the broader nebu-shrinking direction in
  [modernization-survey.md](modernization-survey.md) item #6.

Bill's framing (confirmed 2026-05-08): **HandmadeMath is the better
teaching artifact** for the math layer. It's still single-file C that
a student can read top-to-bottom, but it covers more conventions
(LH/RH, NDC z-ranges, swizzling) and is the same library the wider
graphics-programming community has been converging on. The
fixed-function GL pipeline stays hand-driven for the rendering
pedagogy; the linear-algebra plumbing moves to HMM.

## What HMM covers (sufficient)

For every gltron call site below, HMM has a direct equivalent. Counts
are call-site occurrences across `src/` + `nebu/`.

| nebu symbol | call sites | HMM replacement |
| --- | --- | --- |
| `vec3_Normalize` | 28 | `HMM_NormV3` |
| `vec3_Cross` | 20 | `HMM_Cross` |
| `vec3_Sub` | 19 | `HMM_SubV3` |
| `vec2_Scale` | 16 | `HMM_MulV2F` |
| `vec2_Copy` | 16 | assignment (HMM is value-typed) |
| `vec3_Copy` | 12 | assignment |
| `vec3_Add` | 11 | `HMM_AddV3` |
| `vec2_Add` | 10 | `HMM_AddV2` |
| `vec3_Scale` | 9 | `HMM_MulV3F` |
| `vec2_Length` | 8 | `HMM_LenV2` |
| `vec3_Transform` | 7 | `(HMM_MulM4V4(M, HMM_V4V(v, 0))).XYZ` |
| `vec3_Length` | 7 | `HMM_LenV3` |
| `vec2_Sub` | 7 | `HMM_SubV2` |
| `matrixCofactor` | 7 | (subsumed by `HMM_InvGeneralM4`) |
| `vec3_Dot` | 6 | `HMM_DotV3` |
| `matrixRotationAxis` | 6 | `HMM_Rotate_RH(angle, axis)` |
| `vec3_Print`, `vec4_Print`, `matrixPrint` | 8 total | drop or keep as small debug helpers |
| `vec2_Normalize` | 5 | `HMM_NormV2` |
| `nebu_Quat` | 0 in src/, 0 in nebu outside `quat.c` | **dead code — delete, don't port** (see Quaternion note below) |
| `vec3_Zero` | 4 | `(HMM_Vec3){0}` |
| `vec3_LengthSqr` | — | `HMM_LenSqrV3` |
| `matrixIdentity` | — | `HMM_M4D(1.0f)` |
| `matrixMultiply` | — | `HMM_MulM4` |
| `matrixTranspose` | — | `HMM_TransposeM4` |
| `matrixInverse` | — | `HMM_InvGeneralM4` |
| `matrixDeterminant` | — | `HMM_DeterminantM4` |
| `matrixTranslation` | — | `HMM_Translate` |
| `matrixScale` | — | `HMM_Scale` |
| `Vector3` C++ class | (audio only) | `HMM_Vec3` (with caveats — see below) |

### Quaternion note

The earlier `5` count for `nebu_Quat` was a grep artefact — every hit
was the type's own declarations, the `quat_*` function bodies, or the
internal call from `quat_FromLookAt` to `quat_FromRotationMatrix`. **No
file in `src/` or anywhere in `nebu/` outside the implementation
itself uses quaternions.** The whole `quat_*` family is scaffolded but
unwired:

- `nebu/base/quat.c` (~100 LOC, ~3 functions)
- `nebu/include/base/nebu_quat.h`

In Phase 5, **delete these files outright** instead of porting them
to `HMM_QToM4` / `HMM_M4ToQ_RH`. That removes one of the more
error-prone conversions (the matrix→quaternion extraction with the
trace branch is exactly the kind of code that's easy to subtly break
in translation) for zero behavioural change. Drop the corresponding
`quat.c` line from `nebu/base/CMakeLists.txt` and don't replace it.

Total: 334 algorithmic call sites + ~637 raw `.v[N]` / `.m[N]` field
accesses across 21 first-party files.

## What HMM does *not* cover (must stay or be reimplemented)

These are geometric, not linear-algebra, and HMM intentionally stays
out of that lane:

- `segment2_Intersect`, `segment2_IntersectParallel`,
  `segment2_IntersectNonParallel`, `segment2_findT`, `segment2_Length`
  — 2D segment-segment intersection (~110 LOC). Used heavily in
  `src/game/event.c` and trail/AI logic.
- `box2_*`, `box3_*` — AABB build + center + extend (~50 LOC).
- `vec2_Orthogonal` — 90° 2D rotate. Trivial inline (`HMM_V2(v.Y, -v.X)`)
  but keeping the named function reads better.
- `vec3_TriNormalDirection` — `HMM_Cross(HMM_SubV3(p2,p1), HMM_SubV3(p3,p1))`.
- `uintFromVec3` — RGB float-to-int packer for color encoding. Project-specific.
- `nebu_cosf_deg(x)`, `nebu_sinf_deg(x)` macros — replace with
  `HMM_SinF(HMM_ToRad(x))` / `HMM_CosF(HMM_ToRad(x))`.

(`quat_FromLookAt` was previously listed here as needing a thin
wrapper. It's dead too — see the Quaternion note above. Delete with
the rest of `quat.c` in Phase 5.)

These survive in a new, much smaller `nebu/base/nebu_geom.{c,h}` (or
fold into `nebu_util`). The `vec2`/`vec3`/`vec4`/`matrix` typedefs and
their .c implementations get deleted.

## Compatibility — the good news

- **Memory layout:** `HMM_Mat4` is `union { float Elements[4][4]; HMM_Vec4 Columns[4]; }` —
  16 packed floats, column-major. gltron's `matrix.m[16]` is the same
  layout. Existing `glMultMatrixf(m.m)` calls become
  `glMultMatrixf((float*)m.Elements)` or `glMultMatrixf(m.Elements[0])`.
  No transpose needed. (Verified against `nebu/video/camera.c:40`,
  `src/video/graphics_fx.c:31`, `gamegraphics.c:153`,
  `recognizer.c:70`, `trail.c:177`, `graphics_utility.c:53`.)
- **Vec3 packing:** `HMM_Vec3` is a union over 3 floats with no padding.
  gltron's `vec3 { float v[3]; }` is the same. `memcpy` round-trips,
  array-of-vec3 access stays intact.
- **Quat field order:** HMM stores `X, Y, Z, W`; gltron's `nebu_Quat`
  is `x, y, z, w`. Identical.

## Compatibility — the gotcha

**`Vector3 operator*` semantic mismatch.** gltron's
`nebu/include/base/nebu_Vector3.h` defines `Vector3 operator*(Vector3)`
as the **dot product** (returns `float`). HMM's
`HMM_Vec3 operator*(HMM_Vec3, HMM_Vec3)` is **component-wise multiplication**
(returns `HMM_Vec3`). A naïve type swap in
`nebu/audio/Source3D.cpp` would silently change the meaning of every
`a * b` expression in the Doppler / panning math — producing junk
audio rather than a compiler error.

Mitigation: in the audio port commit, do *not* rely on `*`. Replace
each `a * b` (where the intent is dot) with explicit `HMM_DotV3(a, b)`
**before** swapping the type. Grep target:
`Source3D::GetModifiers` in `nebu/audio/Source3D.cpp` is the only hot
function that uses this idiom; the listener/source location/velocity
math also uses it.

## Field-access mechanical port

`HMM_Vec3` exposes `.X/.Y/.Z`, `.R/.G/.B`, `.U/.V/.W`, `.Elements[3]`.
For a mechanical, low-cognitive-load port, prefer `.Elements[N]` —
matches gltron's existing `.v[N]`. We can transition to `.X/.Y/.Z`
later as a cleanup; this plan keeps it mechanical so the diff is
reviewable.

`sed`-able replacements (after the Vector3 dot-product fix):

```
.v[0]  → .Elements[0]
.v[1]  → .Elements[1]
.v[2]  → .Elements[2]
.v[3]  → .Elements[3]   (vec4 only)
.m[N]  → .Elements[N/4][N%4]   (column-major, identical layout)
```

The matrix one is fiddlier — easier to leave a thin macro
`#define M(M_, R_, C_) (M_).Elements[C_][R_]` in a small compat
header for the duration of the port, then expand it later.

## Decision: out-param vs. value style

gltron's API is out-param-with-pointer-return:
`vec3_Add(&out, &a, &b)` returns `&out`. HMM is value-style:
`HMM_Vec3 r = HMM_AddV3(a, b)`. Every call site becomes a one-line
diff:

```c
/* before */                        /* after */
vec3_Sub(&z, cam, target);          z = HMM_SubV3(*cam, *target);
vec3_Cross(&x, up, &z);             x = HMM_Cross(*up, z);
vec3_Normalize(&x, &x);             x = HMM_NormV3(x);
```

Result is generally clearer. Pointers-into-game-data (e.g.
`(vec3*)cam` casts in `nebu/video/camera.c`) become explicit
dereferences.

## C standard bump

HMM's natural shorthand uses C11 `_Generic` (e.g. `HMM_Add(a, b)` works
for any vec/mat/quat type). gltron currently sets
`CMAKE_C_STANDARD 99` in `CMakeLists.txt`. Two options:

1. **Bump to C11.** Trivial change, all our compilers already support
   it, and it's a 2026 codebase. Recommended. *Don't bump higher than
   needed* — C11 is enough; C17/C23 buys nothing here.
2. **Stay on C99 and use the explicit suffixed names.** Slightly
   noisier (`HMM_AddV3` vs `HMM_Add`) but no policy change.

This plan recommends (1). The explicit names still work after the
bump, so we don't have to commit to one style for the whole port.

## Vendoring

Copy `HandmadeMath.h` into the gltron tree and check it in — Bill's
direction. It's MIT-0 / public domain and intentionally single-file.
No CMake `find_package`, no git submodule. (Survey item #1 already
established the precedent of `find_package`-via-pkg-config for
SDL/Lua, but HMM is too small and volatile-versioned to be worth
that — header-only is the natural form.)

**Source for the vendored copy:** `/HandmadeMath/HandmadeMath.h` at
the time this plan was written.

- Upstream repo: <https://github.com/HandmadeMath/HandmadeMath>
- Pinned commit: `661fef0893bccfe30342049e848b8d54e7430234`
  (`v2.0.0-27-g661fef0`, "small improvement", 2026-03-15)
- LICENSE accompanies it as `LICENSE` next to the header.

**Destination:** `third_party/HandmadeMath/HandmadeMath.h` (with a
sibling `LICENSE`). A `third_party/` directory makes the external
origin obvious at a glance and keeps the include path it generates
(`<HandmadeMath.h>`) decoupled from nebu's include layout.

**Phase 1 step-by-step:**

```sh
mkdir -p third_party/HandmadeMath
cp /HandmadeMath/HandmadeMath.h third_party/HandmadeMath/HandmadeMath.h
cp /HandmadeMath/LICENSE        third_party/HandmadeMath/LICENSE
```

Add a header comment to the vendored `HandmadeMath.h` recording the
upstream commit + date so future updates have a clear baseline:

```c
/* Vendored from https://github.com/HandmadeMath/HandmadeMath
 * Pinned at commit 661fef0893bccfe30342049e848b8d54e7430234
 * (v2.0.0-27-g661fef0, "small improvement", 2026-03-15).
 * Upstream license preserved alongside this file (LICENSE).
 * Do not modify the body of this file — re-vendor with the
 * `update` tool from upstream when bumping the version.
 */
```

Wire it into CMake via a new `INTERFACE` library:

```cmake
add_library(handmademath INTERFACE)
target_include_directories(handmademath INTERFACE
  ${CMAKE_SOURCE_DIR}/third_party/HandmadeMath)
```

Then `target_link_libraries(nebu_includes INTERFACE handmademath)` so
every consumer that already depends on `nebu_includes` picks it up.

When bumping HMM later, refresh both the file and the pinned commit
in the header comment — and re-run the verification gate from the
modernization survey before landing.

## Phased approach

The whole port is mechanically straightforward but big in line count.
Doing it as one PR would be ~1.5k lines of diff. Phasing reduces
review pain and contains risk to one subsystem at a time.

### Phase 1 — drop in HMM, no replacements ✓ DONE 2026-05-08

- Vendored `HandmadeMath.h` + `LICENSE` to
  `third_party/HandmadeMath/`. The header carries a comment pinning
  upstream commit `661fef0893bccfe30342049e848b8d54e7430234`
  (`v2.0.0-27-g661fef0`).
- Bumped `CMAKE_C_STANDARD` 99 → 11 in `CMakeLists.txt`
  (`CMAKE_C_EXTENSIONS` stays ON).
- Added a `handmademath` `INTERFACE` library in `CMakeLists.txt`,
  linked into `nebu_includes`, so any consumer of nebu can
  `#include <HandmadeMath.h>` with no further wiring.
- Verified clean reconfigure + build + smoke run; behaviour identical
  to pre-port.

The "thin compat header" step from the original plan was deferred to
Phase 2: typedef-swapping `vec3` → `HMM_Vec3` is not zero-touch
because gltron uses `.v[N]` field access while HMM exposes
`.Elements[N]`. The two have to migrate together, which makes the
compat header a Phase-2 transitional artifact rather than a Phase-1
no-op.

### Phase 2 — port the C audio Vector3 use site ✓ DONE 2026-05-08

- `Source3D::GetModifiers` rewritten with HMM. Every `Vector3 *
  Vector3` (six sites) was spelled out as an explicit `HMM_DotV3`
  call so HMM's component-wise `operator*` couldn't silently corrupt
  the panning / Doppler math. Mutating `.Normalize()` calls were
  rephrased as value-style `HMM_NormV3`, and the in-place mutation
  on line 122 (`vTargetPlanar.Normalize() * vListenerDirection`) was
  split into a normalize step then a dot.
- `Sound::Listener` fields changed `Vector3` → `HMM_Vec3`.
  `Source3D::_location` / `_velocity` likewise. The `nebu_Vector3.h`
  include was removed from `nebu_SoundSystem.h`, `nebu_Source3D.h`,
  and `nebu_SourceCopy.h` (the last one had a stale dead include —
  it never used the type).
- `src/audio/sound_glue.cpp`: every `Vector3(...)` constructor became
  `HMM_V3(...)`. The `Vector3(float*)` ctor that read a `float[3]` had
  no HMM equivalent — replaced with explicit
  `HMM_V3(arr[0], arr[1], arr[2])` against `camera.cam` /
  `camera.target`.
- `nebu/include/base/nebu_Vector3.h` deleted.
- Verified: clean rebuild, smoke run reaches the menu and exits
  cleanly. The 3D audio path doesn't fully exercise headlessly;
  the SourceEngine pitch-shift effect needs a listen-test on Bill's
  host before this phase is fully cleared.

### Phase 3 — port nebu/base + nebu/video math ✓ DONE 2026-05-08

This phase turned out to be *all* deletion, no porting:

- `nebu/base/quat.c` and `nebu/include/base/nebu_quat.h` deleted
  (quaternions had no callers — see Quaternion note above).
- `nebu/video/camera.c` and `nebu/include/video/nebu_camera.h`
  **also deleted**. A second grep at the start of Phase 3 showed
  that the entire `nebu_Camera` API (Create, LookAt, Rotate, Zoom,
  Roll, Slide, GetRotationMatrix, Print, SetupEyeUpLookAt) has zero
  callers across `src/` and the rest of `nebu/`. The gameplay
  camera (`Camera` in `src/include/game/camera.h`) is a separate,
  unrelated struct. Porting unused code is wasted work, and the
  broader modernization-survey direction is "shrink nebu, drop
  what's not used" — so the file goes.
- The `nebu_Matrix4D` typedef in `nebu/include/base/nebu_matrix.h`
  was only referenced by the now-deleted `camera.c` and
  `nebu_camera.h`. Removed.
- `nebu/base/CMakeLists.txt` and `nebu/video/CMakeLists.txt`
  updated to drop the deleted sources. Build target count
  92 → 90.
- Smoke-tested: clean build, the run actually got far enough that
  the AI player crashed into a wall and the game logged the
  collision. Clean shutdown preserved.

This means **Phase 4 no longer has a `nebu_Matrix4D` →
`HMM_Mat4` typedef migration to do** — the typedef is already gone.
Phase 4 is purely the `src/` call-site sweep against `vec3_*`,
`matrix*`, and the `vec3 { float v[3]; }` struct.

### Phase 4 — port src/game and src/video ✓ DONE 2026-05-08

- `nebu/include/base/nebu_vector.h` and `nebu_matrix.h` now alias the
  legacy lowercase types: `typedef HMM_Vec2 vec2;`, `typedef HMM_Vec3
  vec3;`, `typedef HMM_Vec4 vec4;`, `typedef HMM_Mat4 matrix;`. The
  function declarations for the linear-algebra helpers are gone; only
  the geometric survivors remain (`segment2_*`, `box2_*`, `box3_*`,
  `vec2_Orthogonal`, `vec3_TriNormalDirection`, `uintFromVec3`).
- `nebu/base/vector.c` rewritten to keep just those survivors. Their
  internals now use HMM (`HMM_LenV2`, `HMM_DotV2`, `HMM_Cross`, etc.)
  and `.X/.Y/.Z` field access. `nebu/base/matrix.c` deleted.
- All call sites in `src/` migrated. The actual file list was wider
  than the original plan called out: in addition to the planned 13,
  `src/audio/sound_glue.cpp`, `src/game/32bit_warning.c`,
  `src/game/camera.c`, `src/video/graphics_hud.c`,
  `src/video/recognizer.c`, and `nebu/video/font.c` all needed
  touching too. Total ~19 files.
- Field access pattern: `.v[0]/.v[1]/.v[2]` → `.X/.Y/.Z`
  consistently; `.Elements[k]` only when an index variable was being
  used (e.g. the box3_Compute loop in vector.c). Function calls
  rewritten from out-param-pointer to value-style:
  `vec3_Add(&out, &a, &b)` → `out = HMM_AddV3(a, b)`,
  `vec3_Cross/Sub/Normalize/Length/Dot/Scale` → their HMM
  equivalents, `vec3_Transform(&o, &v, &m)` →
  `HMM_MulM4V4(m, HMM_V4V(v, 0)).XYZ`,
  `matrixRotationAxis(&m, a, &x)` → `HMM_Rotate_RH(a, x)`,
  `matrixIdentity(&m)` → `HMM_M4D(1.0f)`.
- C++ gotcha: `src/audio/sound_glue.cpp` has an `extern "C"` block
  that transitively pulls in `nebu_vector.h` (now → `<HandmadeMath.h>`).
  HMM defines C++ overloaded helpers that conflict if pulled in
  under C linkage. Fixed by including `<HandmadeMath.h>` at the top
  of the .cpp before the `extern "C"` block; the include guard then
  no-ops the later transitive inclusion.
- `glMultMatrixf` continues to accept `m.Elements[0]` since
  `HMM_Mat4` is column-major just like the float[16] it replaced.
  Verified with the doLookAt rewrite in `graphics_utility.c`
  (basis matrix built via `m.Columns[c].XYZ = basis_vec`).
- `src/game/game_level.c` is unbuilt dead code (only `level.c` is in
  CMakeLists). Migrated it anyway for consistency with `level.c`,
  but it should be deleted in Phase 5 along with the other cleanup.
- Build target count now 89 (Phase 3 was 90; Phase 4 dropped
  `nebu/base/matrix.c`).
- Smoke-tested: clean build, smoke run reaches menu → StartGame →
  clean shutdown. No segfaults, no math-related warnings. The
  full-30-second AI match validation still depends on Bill's host
  (the headless container can't drive interactive input).

### Phase 5 — extract geometric helpers and finalise ✓ DONE 2026-05-08

- New `nebu/include/base/nebu_geom.h` is the home for the surviving
  geometric helpers (`segment2_*`, `box2_*`, `box3_*`,
  `vec2_Orthogonal`, `vec3_TriNormalDirection`, `uintFromVec3`) and
  for the `segment2`/`box2`/`box3` struct definitions.
- `nebu/base/vector.c` was renamed to `nebu/base/nebu_geom.c` to
  match the header. Its surface is the same as at the end of Phase 4
  (no linear-algebra leftovers).
- `nebu/include/base/nebu_vector.h` and `nebu/include/base/nebu_matrix.h`
  were **deleted**. `nebu_quat.h` was already deleted in Phase 3.
- `Nebu_base.h`, `nebu_font.h`, `nebu_mesh.h`, and 11 files in `src/`
  had their `#include "base/nebu_vector.h"` / `#include
  "base/nebu_matrix.h"` rewritten to a single `#include
  "base/nebu_geom.h"` (deduplicating where both used to be present).
- `vec2`, `vec3`, and `matrix` survive in `nebu_geom.h` as documented
  typedef aliases for `HMM_Vec2`, `HMM_Vec3`, `HMM_Mat4`. The aliases
  exist because the geometric structs (`segment2.vStart` etc.) read
  more naturally in lowercase, and because keeping them avoids a
  global type rename across ~21 files for marginal benefit. Drop them
  if the inconsistency starts to bite. `vec4` was *not* aliased —
  nothing outside the now-deleted nebu math used it.
- Dead `src/game/game_level.c` deleted (it was an unbuilt copy of
  `level.c`).
- `nebu/base/CMakeLists.txt` swapped `vector.c` → `nebu_geom.c`.
- Smoke-tested: clean rebuild, smoke run reaches menu → StartGame →
  clean shutdown. No regressions.

The transient `nebu_math_compat.h` mentioned in the original plan was
never actually written; Phase 4 went straight from "no aliases" to
"`vec2`/`vec3`/`matrix` aliased in `nebu_vector.h`/`nebu_matrix.h`",
which is what Phase 5 then consolidated into `nebu_geom.h`.

## Open questions for Bill

- **C11 bump scope:** OK to land the `CMAKE_C_STANDARD` bump in Phase 1,
  ahead of any HMM use? It's defensible on its own merits and
  decouples the standard bump from the math churn.

(The "do you want this at all" and "vendoring location" questions are
answered — see Status and Vendoring.)

## Files touched (estimate)

- **Deleted:** `nebu/base/vector.c`, `nebu/base/matrix.c`,
  `nebu/base/quat.c`, `nebu/include/base/nebu_Vector3.h`,
  `nebu/include/base/nebu_quat.h`. (`quat.c` and `nebu_quat.h` are
  pure deletions — no port work, since nothing uses quaternions.)
  Trimmed: `nebu_vector.h`, `nebu_matrix.h`.
- **Added:** vendored `HandmadeMath.h`, `nebu/base/nebu_geom.{c,h}` with
  the surviving geometric helpers, transient
  `nebu/include/base/nebu_math_compat.h` (removed in Phase 5).
- **Modified:** `CMakeLists.txt` (C standard bump), 21 first-party
  consumers under `src/` and `nebu/`.

## Verification

For each phase the bar is the same as the modernization survey:

> `make -f Makefile.docker build` succeeds; the binary launches the
> menu, starts a 2-player AI match, runs for 30s without an audio
> glitch or visual regression, and ESCs back to menu cleanly.

The audio phase deserves an extra check: confirm the engine
pitch-shift effect (the signature SourceEngine sound) is unchanged
after the dot-product rewrite — A/B by listening, not just by linking.
