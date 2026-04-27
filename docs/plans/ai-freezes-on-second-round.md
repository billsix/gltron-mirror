# AI freezes on second round

**Status:** Fixed on branch `billsChanges` (2026-04-27). Awaiting in-game
verification by the user.

## Symptom

On a fresh process, the first round plays normally — computer opponents
steer and try to beat the player. After the first round ends and a new
round begins, the AI cycles stop turning entirely; they just drive forward
until they hit a wall. The only known workaround was to quit the process,
delete `~/.gltron`, and relaunch.

## Root cause

The bug is in `src/game/engine.c`, in `resetPlayerData()`.

- `AI` structs are allocated **once at startup** in `game_CreatePlayers()`
  (called from `init.c:169`) via `malloc(sizeof(AI))`.
- Between rounds, only `game_ResetData()` → `resetPlayerData()` runs;
  `game_FreePlayers` / `game_CreatePlayers` are not called again.
- `game_ResetData()` resets `game2->time.current = 0`.
- `resetPlayerData()` resets `ai->active` and `ai->tdiff = 0` but does **not**
  reset `ai->lasttime`.

The AI's main loop, `doComputerSimple` in `src/game/computer.c:53`, opens with:

```c
if (game2->time.current - ai->lasttime < ai_params.minTurnTime[level]) return;
```

After a round ends, `ai->lasttime` holds the timestamp of the AI's last turn
(say, 75000ms). When the next round starts, `game2->time.current` is back to
0, so the expression is `0 - 75000 = -75000`, which is always less than
`minTurnTime` (≤600ms). The function returns every frame, the AI never picks
a direction, and the cycle drives forward forever. `doComputerActive`
(`computer.c:111`) and the writes in `computer_utilities.c:222,242` use the
same field, so the entire AI is paralyzed.

The first round works because `malloc()` on a freshly-grown heap typically
returns zeroed memory, so `lasttime` happens to be 0 the first time.
**Deleting `~/.gltron` was incidental** — what actually fixed it was killing
the process, which causes the next launch to malloc fresh (zeroed) memory.

## Fix

One-line addition in `src/game/engine.c:resetPlayerData()`, next to the
existing `ai->tdiff = 0;`:

```c
ai->lasttime = 0;
```

This guarantees every round starts with `current - lasttime >= 0`, so the
`minTurnTime` gate behaves correctly from the first frame.

## Why this fix and not a bigger one

- The AI struct could equivalently be `memset` to zero at reset, or
  re-malloc'd per round. But only `lasttime` and `tdiff` carry state across
  rounds, `tdiff` is already reset, and the per-round `ai->active` write
  covers everything else. A targeted one-liner matches the surrounding
  style and avoids touching unrelated code.
- The `segment2` cache fields (`left`, `right`, `front`, `backleft`) are
  recomputed every frame by `ai_getDistances`, so they don't need resetting.

## Verification

- Build (`./autogen.sh && ./configure && make`, or via `make -f Makefile.docker shell`).
- Play a round to completion (crash into a wall) and start a second round.
- Expected: AI opponents continue to turn and engage, no need to delete
  `~/.gltron` or restart the process.
- The user has not yet confirmed in-game; mark this plan **done** once they do.
