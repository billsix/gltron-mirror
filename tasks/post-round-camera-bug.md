# Investigate post-round camera misplacement / misorientation

**Status:** fix applied for the one post-round-specific orientation bug
found (glance state leaking into the observer camera). Left open — not
archived — because it could not be confirmed against Bill's exact repro
(the bug is conditional on crashing while holding a glance key; a sharper
symptom or a GUI run is still wanted to confirm it is *the* thing Bill saw).
See "Resolution (2026-08-03)" at the bottom.
Reported by Bill 2026-05-08 after the HandmadeMath
Phase-5 build. Symptom (Bill's words): *"after a round, there's a
playback section ... It doesn't seem to look at the correct place, or
isn't positioned in the right place, I don't know which."*
**Priority:** 3
**Difficulty:** 5

This is a separate symptom from the Phase-4 doLookAt transposition,
which Bill confirmed fixed for normal play. The post-round path either
exposes a residual bug from the math port or stresses a code path that
the in-round smoke didn't exercise.

## Background — what runs during the post-round playback

`doCameraMovement` in `src/game/camera.c` switches behaviour by player
speed:

```c
for (i = 0; i < game->players; i++) {
  p = game->player + i;
  if (p->data->speed == SPEED_GONE)
    observerCamera(&gPlayerVisuals[i].camera);
  else
    playerCamera(p, i);
}
```

A player's `data->speed` becomes `SPEED_CRASHED` when the crash event
fires, then transitions to `SPEED_GONE` when `exp_radius >=
EXP_RADIUS_MAX` (see `src/game/event.c:351`). So there are two
post-end-of-life states the camera can be in:

1. **SPEED_CRASHED** — the explosion is still expanding. `playerCamera`
   keeps running with the crash-frozen player position.
2. **SPEED_GONE** — explosion done, `observerCamera` puts the camera
   at the recognizer's position. This is the "playback section" Bill
   is most likely talking about.

`observerCamera`:

```c
void observerCamera(Camera* cam) {
  vec2 p, v;
  getRecognizerPositionVelocity(&p, &v);
  cam->cam[0] = p.X;
  cam->cam[1] = p.Y;
  cam->cam[2] = RECOGNIZER_HEIGHT;     // 50
  cam->target[0] = p.X + v.X;
  cam->target[1] = p.Y + v.Y;
  cam->target[2] = RECOGNIZER_HEIGHT - 2;
}
```

Then every frame `drawCam` in `src/video/gamegraphics.c` builds the
view via `doLookAt(cam, target, up)` (with an optional `bIsGlancing`
rotation around `up`).

## Already verified

The investigation prior to writing this plan confirmed by hand-derivation
or diff inspection:

- `doLookAt` in `src/video/graphics_utility.c` matches HMM's own
  `_HMM_LookAt` column structure (basis vectors as rows, encoded as
  `Columns[0] = HMM_V4(x.X, y.X, z.X, 0)` etc.). Verified for an
  example eye/target/up triple.
- `observerCamera`'s only diff from the original is mechanical
  `.v[0]→.X / .v[1]→.Y` substitution.
- `getRecognizerPositionVelocity`'s only diff is the same mechanical
  substitution.
- `box2_Center`, `box2_Diameter` in `nebu/base/nebu_geom.c` match the
  original.
- `engine.c` position-from-data computation matches the original.
- `event.c` crash logic (sets `current->vDirection = HMM_SubV2(v,
  current->vStart)`) matches the original.
- `gamegraphics.c` `drawCam` setup: vec3_Sub→HMM_SubV3, vec3_Normalize→
  HMM_NormV3, matrixRotationAxis→HMM_Rotate_RH, vec3_Transform→
  `HMM_MulM4V4(M, HMM_V4V(v, 0)).XYZ`. All semantically equivalent
  for the values that flow through this path.
- `HMM_Rotate_RH` matches `matrixRotationAxis` element-by-element for
  unit-axis input (`up = {0,0,1}` is unit, so HMM's normalize is a
  no-op).

So the obvious mechanical-translation candidates are eliminated. The
bug, if it's a Phase-4 regression, is somewhere subtler.

## Hypothesis space (ranked by plausibility)

### H1 — `bIsGlancing` rotation persists post-crash

If the user was glancing when they crashed, `cam->bIsGlancing` is `1`.
`observerCamera` doesn't reset it, so the `drawCam` rotation block
applies a **90-radian** rotation (≈ 5 turns) to the lookAt direction.
That'd put the camera looking off in a near-random direction.

This is not a Phase-4 regression — same value flows through the
original code. But the rotation matrix construction goes through HMM
now and might produce a numerically different result for the same
non-trivial angle, even if `up = (0,0,1)` is unit.

**Investigation:** print `cam->bIsGlancing` and the resulting `vTarget`
in `drawCam` while in observer mode. If `bIsGlancing` is non-zero,
that's at least *part* of the issue.

### H2 — `gltron_Mesh_ComputeBBox` rewrite changed `BBox.vSize`

The recognizer position math reads `BBox.vSize.X` (the recognizer
mesh's bounding-box width). I rewrote that function in `model.c`
during Phase 4:

```c
vec3 vMin = *(vec3*)pMesh->pVertices;
vec3 vMax = vMin;
for (i = 0; i < pMesh->nVertices; i++) {
  for (j = 0; j < 3; j++) {
    float f = pMesh->pVertices[3 * i + j];
    if (vMin.Elements[j] > f) vMin.Elements[j] = f;
    if (vMax.Elements[j] < f) vMax.Elements[j] = f;
  }
}
pMesh->BBox.vMin = vMin;
pMesh->BBox.vSize = HMM_SubV3(vMax, vMin);
pMesh->BBox.fRadius = HMM_LenV3(pMesh->BBox.vSize) / 10;
```

The original used `vec3_Copy(&vMin, (vec3*)pMesh->pVertices)` then
`vec3_Sub(&vSize, &vMax, &vMin)`. Should be identical, but the
`*(vec3*)pMesh->pVertices` initial read assumes 3-float packing — if
HMM_Vec3 ever picked up alignment padding it wouldn't, but the
sizeof check at the start of Phase 4 already verified packing matches.
Worth re-confirming for this specific pointer cast.

**Investigation:** print `pMesh->BBox.vSize` and `pMesh->BBox.vMin`
once at startup; compare to known-good values (or to a checkout at
`8e019b5e` running the same way). Also: this only affects the
recognizer's `max` computation, which feeds `rec_boundry`. If
`BBox.vSize.X` is way off, the recognizer flies wildly.

### H3 — `playerCamera` during SPEED_CRASHED is bogus

`getPositionFromData` returns `vStart + vDirection`. After crash,
`vDirection` was reset by `event.c` to `crashpoint - vStart`, so the
position is the crash point. Camera params (r, phi, chi) keep
spinning per `cam->movement[CAM_PHI] += CAM_SPEED * dt` (in
CAM_TYPE_CIRCLING). This makes the camera orbit the crash point
during the explosion. *That's the intended behaviour* — but if
`getPositionFromData` returns weird values post-crash because the
trail buffer math is off, the orbit centre would be wrong.

**Investigation:** print `data->trails[trailOffset].vStart` and
`.vDirection` immediately after a crash. Compare to expected.

### H4 — A `cam->cam`/`cam->target` write got truncated to 2 components

`observerCamera` writes all three. `playerCamera` ends with `memcpy`s
of `dest`/`tdest` (float[3]). Both look complete. But other code
might write `.X/.Y/.Z` and miss one component, leaving stale data.
Worth grepping for any incomplete migration.

**Investigation:** `git -C /gltron diff db7141a0..HEAD -- src/game/
src/video/` and look for `cam->cam[...]` / `cam->target[...]`
assignments that don't set all three components.

### H5 — The post-round renderer takes a different code path I didn't audit

After the round ends, the engine sets `game->pauseflag =
PAUSE_GAME_FINISHED`. There may be an alternate render path or
overlay (a "winner" screen, scoreboard, etc.) that uses different
camera math. Worth grepping for `PAUSE_GAME_FINISHED` consumers.

**Investigation:** `grep -rn PAUSE_GAME_FINISHED` in `src/`. Trace
which code paths render in that state.

## Investigation order

When picking this up:

1. **Get a sharper symptom from Bill first.** "Wrong position vs
   wrong orientation" makes a big difference in which hypotheses
   to chase. Ideally a screenshot, or a description like "camera is
   inside the floor" / "camera is rotated 90° from where it used to
   be" / "looking at empty space far from the action".
2. **Add a debug print** at the top of `drawCam` for player 0 only,
   dumping `cam->cam`, `cam->target`, and `cam->bIsGlancing`, gated
   on `game->player[0].data->speed == SPEED_GONE`. Run a round, let
   it end, capture the prints. Compare to the same prints on a
   pre-Phase-4 build (`git checkout 8e019b5e -- src/`, build, run,
   diff the prints).
3. **If the values match but the picture is still wrong**, the bug
   is in the rendering of the matrix, not the values. Re-check
   `doLookAt` against an HMM-native `HMM_LookAt_RH(cam, target, up)`
   call (substitute and see if behaviour changes).
4. **If the values diverge**, walk back through the call chain
   (`observerCamera` → `getRecognizerPositionVelocity` → `box2_*` /
   `BBox.vSize.X`) printing intermediates until the divergence is
   localized.
5. Cross-check H1 by force-clearing `cam->bIsGlancing = 0` at the
   top of `observerCamera` and re-testing. If the camera now looks
   right, the post-crash `bIsGlancing` carry-over is the problem
   (and it's a pre-existing issue, not a regression).

## Pre-existing bug (not the cause) — already fixed

`getRecognizerAngle` in `src/video/recognizer.c` used to read the
velocity's X component into both `dxval` and `dyval`. **As of the current
tree it already reads `velocity->Y` for `dyval`** (recognizer.c:37), so
this is fixed — leaving the note for history only. It only affected the
recognizer's *visual orientation* (the `glRotatef` in `drawRecognizer`),
not the observer camera's lookAt vector, so it was never the cause of the
camera mis-placement anyway.

## Resolution (2026-08-03)

**Root cause (of the one post-round-specific orientation bug in the code):
`observerCamera` never clears `cam->bIsGlancing`.**

Walk of the post-round path confirms the observer camera is the *only*
post-round-specific camera code (during play the human follows their own
cycle via `playerCamera`; `drawRecognizer` is drawn during play too, so a
recognizer-mesh/BBox regression — H2 — would show up mid-round, not only
after, ruling H2 out as the post-round symptom). Within that path:

- `drawCam` (`src/video/gamegraphics.c:462-464`) rotates the look
  direction by `90 * cam->bIsGlancing` every frame, in *both* play and
  observer mode.
- `bIsGlancing` is set to `+1` / `-1` on glance-key **down** and back to
  `0` on **up** (`src/input/input.c:114,117,128,131`).
- Only `initCamera` clears it (`camera.c:152`). `observerCamera` did not.

So if the player crashes **while holding a glance key**, `bIsGlancing`
stays non-zero into the post-round playback, and the recognizer-follow
view is rotated 90 degrees off to the side — camera positioned correctly
(at the recognizer) but "looking at the wrong place." That matches the
"looks at the wrong place OR isn't positioned right, not sure which" half
of Bill's report (position right, orientation wrong).

This is confirmed **pre-existing**, not a HandmadeMath-port regression:
the pre-port `observerCamera` (`6edb5e9^`) also failed to clear
`bIsGlancing`, and `bIsGlancing` was only ever reset in `initCamera` there
too. Matches this doc's H1.

**Related observation (NOT reverted): the port changed the glance angle
magnitude.** Pre-port, `matrixRotationAxis(&m, 90.0f * bIsGlancing, up)`
fed the angle straight into `cos()/sin()`, i.e. **90 radians** (≈116.8°
after mod 2*pi). The port rewrote it as
`HMM_Rotate_RH(HMM_AngleDeg(90.0f * bIsGlancing), up)` — **90 degrees**.
90° is the obviously-intended glance angle, so the port's value is *more*
correct; I did not revert it. It only changes the *magnitude* of the stale
rotation, not whether the leak happens. Left as-is by design.

**Fix applied:** clear `cam->bIsGlancing = 0` at the top of
`observerCamera` in `src/game/camera.c`, mirroring `initCamera`, with a
comment explaining why. One line + comment; surgical, matches surrounding
style.

**Verification:**
- `src/game/camera.c` syntax-checked with
  `gcc -std=c11 -fsyntax-only -Isrc/include -Inebu/include
  -Ithird_party/HandmadeMath src/game/camera.c` — clean (exit 0), both
  before and after the edit.
- `doLookAt` re-derived against the pre-port version (`6edb5e9^`)
  column-by-column — the port is a faithful translation (columns match),
  so the lookAt matrix is not a suspect. Recorded here so it is not
  re-chased.
- **Not run:** full `make -f Makefile.docker image` container build
  (heavy; shared 32G store with other agents) and the Xvfb GUI runtime
  check. So the fix is verified at the source/compile level and by code
  tracing, **not** by watching the post-round camera in a live run.

**Remaining uncertainty / follow-up for Bill:** the fix is correct on its
own merits (the playback camera should never be in a glance state), but I
could not confirm it reproduces Bill's exact symptom, because the leak
only triggers when you crash mid-glance — whereas Bill's report reads as
happening every round. If the misplacement still shows *without* glancing
at crash time, the next suspect is H1's numeric sibling ruled unlikely
here (H2 BBox / H3 crash-position), and the right next step is the debug
print in step 2 of "Investigation order" on a round ended *without*
touching a glance key.

**Runtime confirmation is now its own task: `tasks/verify-post-round-camera-fix.md`**
(Bill, 2026-08-03) — the source-level fix is staged; that task covers watching it
in a live/Xvfb run, especially the crash-without-glancing case.

## Decision criteria for "done"

The bug is identified when there's a reproducible, single-line root
cause and a fix that restores the post-round camera to the same
position + orientation as the pre-port build (`8e019b5e`). At that
point either:
- it's a Phase-4 regression and gets fixed in `src/` (then a follow-up
  commit), or
- it's a pre-existing bug exposed by the port, in which case fix
  it deliberately (with a note in the commit message that the
  original was already wrong).
