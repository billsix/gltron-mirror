# gltron — camera & post-round observer pipeline

**Reference document** — how the camera is placed each frame, and the post-round switch to the observer
("recognizer") camera. gltron's first reference doc. Not a task; update in place. Created 2026-08-27
(William Emerison Six <billsix@gmail.com>) from a direct read (anchors re-verified). Companion to the
`post-round-camera-bug` investigation.

## Per-frame camera dispatch

- **`doCameraMovement` (`src/game/camera.c:307`)** runs per player each frame and dispatches on the
  player's `speed`: `if (p->data->speed == SPEED_GONE)` → **`observerCamera`** (`camera.c:315`), else →
  **`playerCamera`** (`camera.c:317`). (`observerCamera` at `camera.c:156`, `playerCamera` at
  `camera.c:168`.)
- **`playerCamera`** places the camera relative to the live player (position/heading + configured
  follow distance/height).
- **`observerCamera`** places the camera at the **recognizer** (the flying observer craft):
  `getRecognizerPositionVelocity` for position/velocity, `RECOGNIZER_HEIGHT` for altitude.

## The post-round transition (player crash → observer)

- A crashed player's explosion grows until `exp_radius >= EXP_RADIUS_MAX`
  (`src/game/event.c:349-351`), which advances the "done exploding" state that flips the player toward
  **`SPEED_GONE`** — after which `doCameraMovement` routes that player to `observerCamera`. So the
  "post-round" camera *is* the observer/recognizer camera.

## View-matrix construction

- `drawCam` (`src/video/gamegraphics.c`) builds the view via `doLookAt`
  (`src/video/graphics_utility.c`), whose column structure matches HandmadeMath's `_HMM_LookAt`.

## Recognizer orientation (and a now-stale bug claim)

- `getRecognizerAngle` (`src/video/recognizer.c`) derives the recognizer's facing from its velocity:
  `dxval = velocity->X`, **`dyval = velocity->Y`** (`recognizer.c:36`).
- **Correction (verified 2026-08-27):** the `post-round-camera-bug` task doc flags this line as a
  copy-paste bug (`dyval = velocity->X`). **The current code reads `velocity->Y` — the bug is already
  fixed.** Recording this here is the durable value; the task doc's "pre-existing bug worth noting" is
  stale and has been corrected.

## Follow-on

`tasks/post-round-camera-bug.md` — the root-cause investigation of the observer-camera misplacement now
has this accurate pipeline map to work against. (That fix is runtime-gated + needs a sharper symptom from
the maintainer — it stays its own task; this doc just removes the stale `recognizer.c` claim and grounds
the pipeline.)

## Cross-links

- `tasks/post-round-camera-bug.md` (the investigation this grounds).
