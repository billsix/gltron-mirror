# Trim gltron's CLAUDE.md (6,420 B ≈ 1.6K tok, loaded every session)

**Status:** Done — trimmed 2026-09-13 (pending archive after the work commit)
**Priority:** 6
**Difficulty:** 3

## Result (2026-09-13)
`CLAUDE.md` **6,420 B → 5,338 B** (−1,082 B). The "Completed" block (esp. the ~1 KB
HandmadeMath post-mortem) was collapsed to one-line pointers into `tasks/archive/**` — the
full post-mortems (rationale, phases, deletions) already live in those archived task docs, so
nothing was lost. The live action buried in the HandmadeMath "Outstanding" note (Bill's host
listen-test of the SourceEngine pitch effect, the Phase-2 `Vector3 *` → `HMM_DotV3` rewrite)
was split into a new **`tasks/verify-handmademath-audio-pitch.md`** (status `blocked`) before
trimming, and added to Active. Also fixed: the stale Active entry pointing at
`tasks/modernization-survey.md` (already archived 2026-08-27) → moved to the Completed
pointer list.

## BLUF
gltron's `CLAUDE.md` is 6,420 B (~1.6K tok) and re-splices into context every turn. Most
of it is lean and operational, but the "Tasks / plans" section has grown into a full
task-index-plus-history — the biggest offender being a ~1 KB completed-task post-mortem
(the HandmadeMath port) that already lives verbatim in its archived task doc. Trim the
completed-work prose down to one-line pointers; the archive is the record.

## Context
- **Why:** `CLAUDE.md` loads on every session/turn. Method + measured numbers:
  runCrushInContainer `tasks/reference/crush-context-assembly.md`.
- **Current size:** 6,420 B ≈ 1.6K tok. **@-imports:** none.
- **Convention:** `CLAUDE.md` stays lean and operational; history / post-mortems /
  rejected alternatives move to (or stay in) `tasks/` archives and `tasks/reference/`.
- **Existing `tasks/reference/` docs:** `camera-and-observer-pipeline.md` (2,656 B) —
  per-frame camera dispatch + post-round observer switch.

## Stay vs move (section-by-section)

| section (heading) | ~bytes | Verdict | Destination |
|---|---|---|---|
| Intro / "what this is" (teaching artifact, no-shaders) (L1–7) | ~500 | STAY | CLAUDE.md |
| Status (SDL3, platforms, audio, Lua, CMake, podman) (L9–25) | ~1,100 | STAY (tighten) | CLAUDE.md (operational) |
| Layout (L27–41) | ~900 | STAY | CLAUDE.md |
| Runtime config (`~/.gltronrc`) (L43–47) | ~300 | STAY | CLAUDE.md (operational) |
| Tasks/plans — Active + reference pointer (L49–65) | ~1,200 | STAY (trim) | CLAUDE.md — keep Active pointers + the reference-doc note |
| Tasks/plans — Completed list, esp. HandmadeMath block (L66–73) | ~1,600 | TRIM → one line each | detail already in `tasks/archive/**`; keep bare pointers |
| Tasks — housekeeping paragraph (L75–79) | ~500 | STAY (tighten) | CLAUDE.md |
| Conventions (C11, teaching-artifact, minimal fixes) (L81–88) | ~500 | STAY | CLAUDE.md |

## Projected result
- Collapsing the "Completed" block (L66–73) to one-line-per-task pointers and tightening
  Status should cut roughly **1.5–2 KB**, landing near **~4.5 KB (~1.1K tok)** — mostly
  Status, Layout, Active tasks, and Conventions.
- No information lost: the completed entries duplicate their archived task docs.

## Open questions
1. The HandmadeMath "Outstanding" note (L73) carries a real live action — "Bill should
   listen-test the SourceEngine pitch effect to validate the Phase-2 audio rewrite." If
   its parent completed-entry is trimmed away, that action needs a home. **Recommendation:**
   split it into a small `tasks/verify-handmademath-audio-pitch.md` (proposed) so it isn't
   lost, then trim the block.

## Related
- runCrushInContainer `tasks/reference/crush-context-assembly.md` — measurement + method.
- `tasks/reference/camera-and-observer-pipeline.md`; `tasks/archive/2026/05/10/handmademath-port.md`.
