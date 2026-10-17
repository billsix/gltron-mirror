# Latent bugs surfaced by the C modernization sweep

**Status:** in progress — **Bug 1 FIXED 2026-10-17** (commit e0ce0fb8, clang -Werror clean); **Bug 2
needs a maintainer gameplay decision** (see below — it is a behavior change, not a clear safety fix).
**Priority:** 5
**Difficulty:** 3
**Created:** 2026-10-17 (William Emerison Six <billsix@gmail.com>)
**Source:** `tasks/archive/2026/10/17/c-modernization-sweep.md` (Findings & follow-ups) —
these are **pre-existing** bugs the sweep *surfaced* (via `-Wshadow`/`-Wunused`) and deliberately
**preserved behavior-identically**; it did not fix them (changing behavior wasn't that task's job).

## BLUF

Two long-standing latent bugs in first-party code, each needing a **decide-before-fix**
investigation: confirm whether the current (arguably broken) behavior is reachable / relied upon
before changing it — on a teaching artifact a "bug" can be an intentional demonstration, so verify,
don't assume. "Done" = each bug is either fixed (warning `-Werror` build clean under clang+gcc, the
UBSan+ASan timedemo gate and `make image RUN_SANITIZERS=1` still green) **or** confirmed benign and
documented in a one-line code comment. Neither is urgent (both predate the sweep and the game runs),
but bug 1 is a latent buffer overflow, so it leads.

## Bug 1 — `scripting_CopyStringResult` can overflow the caller's buffer

**Where:** `nebu/scripting/scripting.c`, `scripting_CopyStringResult(char* s, int len)`
(the `if (size > len) { copy = len; status = 2; } else copy = size;` block, then the `memcpy`).

**What's wrong:** the function computes a clamped length `copy = min(size, len)` and sets
`status = 2` to signal truncation, but then does `memcpy(s, lua_tostring(L, -1), (size_t)size)` —
using the **unclamped** `size`, not `copy`. If the Lua string (`size`) is larger than the caller's
buffer (`len`), it writes `size` bytes into an `s` that holds only `len` → **buffer overflow**. The
clamp is dead code (that is why the sweep's `-Wunused-but-set-variable` flagged `copy`; it is
currently kept alive with `[[maybe_unused]]`).

**Decision needed before fixing:**
- Enumerate callers of `scripting_CopyStringResult` and the `(s, len)` they pass. Is `len` ever
  smaller than a Lua string that reaches here (i.e. is the overflow actually reachable), or do all
  callers over-provision so it never triggers? (`git grep -n scripting_CopyStringResult`.)
- Confirm the intended contract: truncate-and-signal (status 2) is clearly the design.

**FIXED 2026-10-17 (commit e0ce0fb8).** Confirmed reachable: `gui.c:250` calls
`scripting_CopyStringResult(pMenuName, 200)` (fixed 200-byte buffer), so a menu name > 200 bytes
overflows; the `sizeof`-passing callers (`gui.c:325`/`338`) were already safe. Applied the candidate
fix — `memcpy(..., (size_t)copy)` and dropped the `[[maybe_unused]]` (both `size` and `copy` are now
used). Behavior changes only in the formerly-overflowing case (now truncates + returns status 2),
matching the clamp's evident intent. The sibling `scripting_GetStringResult` `malloc`s `size` and is
fine (unchanged). clang -Werror clean, binary links.

## Bug 2 — `crashTestPlayers` always returns 0

**Where:** `src/game/event.c`, `crashTestPlayers(int i, const segment2* movement)`; sole caller at
the `crash = crashTestPlayers(i, &movement);` line.

**What's wrong:** the inner per-opponent flag `jCrash` (renamed from a shadowing inner `crash` by
the sweep's `-Wshadow` fix) drives the inner `break`, but the function ends with `return crash;`
where the **outer** `crash` is initialized to 0 and never assigned — so the function always returns
0. The actual crash is dispatched *inside* the function via `createEvent(i, EVENT_CRASH)`, so the
game's crash handling likely runs through the event system regardless of the return value.

**Caller investigated 2026-10-17 — the return IS used, so this is a real (subtle) gameplay bug:**
`src/game/event.c:325-338`:
```c
crash = crashTestPlayers(i, &movement);
if (crash) { printf("player %d crashed into other players\n", i); ... }   // never runs (crash==0)
if (!crash) {                                                             // always runs
  crash = crashTestWalls(i, &movement); ...
}
```
Because `crashTestPlayers` always returns 0: (a) the "crashed into other players" branch never runs,
and (b) **wall-collision is tested even after a player-crash**. The crash itself still fires (the
internal `createEvent(i, EVENT_CRASH)`), but the wall test can then fire a *second* `EVENT_CRASH` in
the same step. So fixing it (make it return the detected state) is a **behavior change to crash/wall
handling**, not a safety no-op.

**DECISION FOR THE MAINTAINER (not applied — this is the "is the broken behavior load-bearing?"
call on a teaching artifact):**
- Option A (recommended) — restore the intent: set the function-level `crash` when a player-crash is
  found (e.g. `crash = 1;` where `jCrash = 1;`, or `return jCrash` appropriately) so the caller skips
  the wall test after a player-crash and the double `EVENT_CRASH` can't happen. Small, matches the
  caller's evident `if (!crash)` design. **Changes gameplay** (verify a round looks right).
- Option B — declare it vestigial: if the current behavior (always also wall-testing) is what the
  game has always done and is considered fine, leave the logic and either make the function `void` or
  add a one-line comment that the return is intentionally unused. No gameplay change.
Not fixed pending your call, since either way it is a deliberate gameplay decision.

## Verification

Same nets as the sweep: `-Werror` build clean (clang + gcc); UBSan-trap + ASan timedemo gate
ALL PASS; authoritative `make -f Makefile.docker image RUN_SANITIZERS=1` green. A behavior change
(bug 2, if the return is wired up) also wants a runtime sanity check of crash handling in a real
round — a hands-on check only the maintainer can do.
