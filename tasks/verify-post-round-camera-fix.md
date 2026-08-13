# Verify the post-round camera fix at runtime

**Status:** proposed — needs a runtime/GUI run to confirm
**Priority:** 3
**Difficulty:** 3
**Created:** 2026-08-03

## Goal

Confirm that the `bIsGlancing` clear added to `observerCamera` (`src/game/camera.c`)
actually fixes Bill's reported post-round camera symptom in a live run — the source-level
fix is verified (compiles clean, code-traced) but was **not** watched in a running game.
See the diagnosis and fix in `tasks/post-round-camera-bug.md` (root cause: the glance-state
flag leaked into the observer camera when a player crashed while holding a glance key).

## The one open question this must answer

The fix only triggers when the player crashes **while holding a glance key**, but Bill's
report reads as happening *every round*. So:

- **Crash while NOT touching a glance key, and watch the post-round playback camera.**
  - If it now looks correct → the glance-state leak was the whole story; fix confirmed, archive.
  - If it is *still* misplaced/misoriented without glancing → the leak was only part of it,
    and the next suspects are H2 (recognizer BBox `vSize`) / H3 (crash-position), via the
    debug-print in "Investigation order" step 2 of `tasks/post-round-camera-bug.md`.

## How to verify (runtime)

- Headless in-sandbox: run the game under `Xvfb` and screenshot the post-round playback
  (per Bill's sandbox GUI-capture recipe — Xvfb `:99`, share `/tmp/.X11-unix`, software GL),
  ending a round both with and without a held glance key; compare the camera framing. Note
  gltron's own gate is the podman image build (`make -f Makefile.docker image`), which was
  skipped during the fix to spare shared RAM — a real build is needed here.
- Or on Bill's host: play a round, crash without glancing, eyeball the playback camera; then
  crash mid-glance and confirm it is no longer rotated 90° off.
- Gold reference for "correct": the pre-port build `8e019b5e` (post-round camera framing).

## Notes / decisions

- Split out from `tasks/post-round-camera-bug.md` (Bill, 2026-08-03) so the runtime
  confirmation is tracked separately from the diagnosis+fix (which is staged, not yet
  runtime-confirmed).
