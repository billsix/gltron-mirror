# Latent bugs surfaced by the C modernization sweep

**Status:** proposed — needs go-ahead
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

**Candidate fix (if confirmed):** `memcpy(s, lua_tostring(L, -1), (size_t)copy);` and drop the
`[[maybe_unused]]` on `copy`. Then re-run the gates. (Compare with the sibling
`scripting_GetStringResult`, which `malloc`s `size` itself and is fine.)

## Bug 2 — `crashTestPlayers` always returns 0

**Where:** `src/game/event.c`, `crashTestPlayers(int i, const segment2* movement)`; sole caller at
the `crash = crashTestPlayers(i, &movement);` line.

**What's wrong:** the inner per-opponent flag `jCrash` (renamed from a shadowing inner `crash` by
the sweep's `-Wshadow` fix) drives the inner `break`, but the function ends with `return crash;`
where the **outer** `crash` is initialized to 0 and never assigned — so the function always returns
0. The actual crash is dispatched *inside* the function via `createEvent(i, EVENT_CRASH)`, so the
game's crash handling likely runs through the event system regardless of the return value.

**Decision needed before fixing:**
- Inspect the caller: does anything branch on the returned `crash` (e.g. `if (crash) …`), or is the
  assignment effectively dead because `EVENT_CRASH` already does the work? If the return is unused
  in effect, this is harmless vestigial code; if a caller branch depends on it, it is a real
  gameplay bug (crash-side logic that never runs).

**Candidate fix (if the return is meant to be used):** return the detected state (e.g.
`return jCrash;` hoisted, or set the outer `crash` when a crash is found). If vestigial: either
change the signature to `void` or leave it with a one-line comment that the return is unused and the
event system is authoritative. Either way, preserve the existing crash *behavior* exactly.

## Verification

Same nets as the sweep: `-Werror` build clean (clang + gcc); UBSan-trap + ASan timedemo gate
ALL PASS; authoritative `make -f Makefile.docker image RUN_SANITIZERS=1` green. A behavior change
(bug 2, if the return is wired up) also wants a runtime sanity check of crash handling in a real
round — a hands-on check only the maintainer can do.
