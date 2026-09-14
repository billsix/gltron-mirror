# Verify HandmadeMath audio pitch effect (SourceEngine pitch)

**Status:** blocked
**Priority:** 7
**Difficulty:** 2
**Blocked on:** a host listen-test only Bill can run (no display/audio in the sandbox).
**Recheck:** ask Bill whether he has run the listen-test; when he confirms the SourceEngine
pitch effect sounds correct, close this task.

## BLUF
The HandmadeMath port (all 5 phases shipped 2026-05-08) has exactly one outstanding
follow-up: Bill should **listen-test the SourceEngine pitch effect on his host** to validate
the Phase-2 audio rewrite that replaced the `Vector3 *` dot-product path with `HMM_DotV3`.
Everything else in the port is on disk and verified by smoke build + menu-launch run. Split
out of `CLAUDE.md` (2026-09-13 trim) so this live action isn't lost when the completed-task
post-mortem was collapsed to a one-line pointer.

## Context
- **Parent (completed) task:** `tasks/archive/2026/05/10/handmademath-port.md` — the full
  post-mortem. Its "Outstanding" note (lines 9–12) is the source of this action: "the only
  follow-up is Bill's listen-test of the SourceEngine pitch effect on his host (the Phase 2
  dot-product rewrite). Everything else is on disk and verified by smoke build + menu-launch
  run."
- **What changed:** Phase 2 of the port rewrote the audio pitch math from the hand-rolled
  `Vector3 *` dot product to `HMM_DotV3`. The concern (also flagged in the archive around
  line 316) is that the pitch-shift effect needs an ear check, not just a compile/run check.
- **Why a listen-test:** the sandbox has no audio device, so only Bill can confirm the
  effect sounds correct on his host.

## Done when
Bill has listened to the SourceEngine pitch effect in a running gltron on his host and
confirmed it is unchanged / correct after the HandmadeMath dot-product rewrite.
