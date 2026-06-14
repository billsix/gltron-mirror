# Investigate post-round camera misplacement / misorientation

**Status:** open. Reported by Bill 2026-05-08 after the HandmadeMath
Phase-5 build. Symptom (Bill's words): *"after a round, there's a
playback section ... It doesn't seem to look at the correct place, or
isn't positioned in the right place, I don't know which."*

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

## Pre-existing bug worth noting (not the cause)

`getRecognizerAngle` in `src/video/recognizer.c:35` reads the
velocity's X component into both `dxval` and `dyval`:

```c
float dxval = velocity->X;
float dyval = velocity->X;   // should be ->Y
```

This was identical in the original (`velocity->v[0]` for both). It
only affects the recognizer's *visual orientation* (the `glRotatef`
in `drawRecognizer`), not the observer camera's lookAt vector. Worth
fixing for cosmetic reasons, but not the cause of the camera
mis-placement Bill is reporting.

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
