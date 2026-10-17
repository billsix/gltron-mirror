# Behavior-preserving C modernization sweep

**Status:** ready — scope decisions settled 2026-10-17; awaiting go-ahead to start (begin at Phase 0)
**Priority:** 6
**Difficulty:** 5
**Created:** 2026-10-17 (William Emerison Six <billsix@gmail.com>)
**Decisions settled:** 2026-10-17 (William Emerison Six <billsix@gmail.com>) — see Decisions below

## BLUF

Sweep the ~14 KLOC of **first-party** C (`src/`, `nebu/`; pinned at **C23**, nothing
under `third_party/` and no HandmadeMath edits) toward the maintainer's shared C
coding standard **class-by-class and behavior-preservingly** — fixing
the real safety bugs the standard names (unbounded `sprintf`, unchecked
`malloc`, a leaked symbol) and adopting the safe idioms (`bool`, narrow `const`,
fixed-width types *only where width matters*), while **explicitly opting out** of
every place the "correct" transform would damage this deliberate teaching
artifact. "Done" = the raised warning level and the sanitizer gate are green,
every transform is either verified by the gate or marked as unverified, and
nothing about the fixed-function-OpenGL pedagogy or the read-top-to-bottom shape
has changed. This is **not** a stylistic rewrite and **not** the GL core-profile
port.

## Context (read first, cold-start orientation)

Read these before touching code:

1. The maintainer's **C coding standard** — canonical at
   `github.com/billsix/runClaudeInContainer`, `tasks/reference/c-coding-standard.md`
   (mounted in-session at `~/.claude/reference/c-coding-standard.md`). It is split
   (a) what the toolchain enforces mechanically vs (b) judgment calls. This sweep
   is "apply (b), and turn on the tools that do (a)."
2. The companion **C modernization-sweep** doc — same repo,
   `tasks/reference/c-modernization-sweep.md` (`~/.claude/reference/c-modernization-sweep.md`).
   It is the *other direction*: the preservation hazards where an idiom from the
   standard must NOT be applied to existing code, plus sweep verification and
   mechanics. This task is a direct instance of that method.
3. This repo's `CLAUDE.md` — the binding constraint: **gltron is an intentional
   teaching artifact** ("Don't add new abstractions or 'modernize' code as
   drive-by cleanup … Match the surrounding style. Prefer minimal, surgical fixes
   over refactors."). The user's explicit request for this task authorizes a
   *scoped* modernization; it does not loosen the teaching-artifact rule, it
   scopes the sweep to safety/correctness + safe idioms and forbids churn.
4. `tasks/c23-port-investigation.md` and `tasks/archive/2026/10/17/add-sanitizer-gate.md` — the two
   existing tasks this one sits between (see Relationship, and open questions 3–4).

### Current state (surveyed 2026-10-17)

- **Baseline:** `CMAKE_C_STANDARD 11`, `CMAKE_C_EXTENSIONS ON` (GNU/POSIX — needed
  for `realpath()` in `nebu/filesystem/findpath.c`; C11 is needed for
  HandmadeMath's `_Generic`). Image compilers: clang 22, gcc 16 (both full C23).
- **Warning level is effectively off:** the only flags are `-Wall -Werror`, behind
  `option(GLTRON_WARN ... OFF)` in `CMakeLists.txt:26,78-82`. None of
  `-Wconversion -Wsign-conversion -Wshadow -Wvla -Wstrict-prototypes
  -Wold-style-definition -Wwrite-strings -Wformat=2` are wired.
- **`.clang-tidy` is configured** (`bugprone-*,clang-analyzer-*,cert-*,readability-*,…`,
  `WarningsAsErrors: ''`) but **nothing runs it** — `entrypoint/format.sh` runs
  `clang-format` only. No Cppcheck. So tier (a) of the standard is unenforced today.
- **Style is classic C89/C99:** top-of-function declarations (`int i; vec2 v;` …),
  `int` flags used as booleans, no `<stdbool.h>` anywhere, 173 `NULL`s, `assert(0)`
  as the error strategy, `switch` cases with redundant `return; break;`.
- **Concrete safety findings (the payload of this sweep):**
  - **Unbounded `sprintf` into fixed buffers** — `src/filesystem/path.c:28-41,74`
    writes into `char[PATH_MAX]` statics / a `malloc(PATH_MAX)` with no width
    bound; `PATH_MAX` falls back to **255** when unset (`path.c:13-16`). Also
    `src/game/scripting_interface.c:219-220`, `src/video/graphics_hud.c`,
    `src/audio/sound_glue.cpp:240`, `src/video/material.c:61` (`strcpy`). 46
    `sprintf`/`strcpy`/`strncpy`/`strcat` call sites total.
  - **Unchecked allocation** — `getPossiblePath()` (`path.c:72-76`) does
    `char* path = malloc(PATH_MAX);` then `sprintf` into it with no NULL check, and
    returns it with no stated ownership ("caller frees"). 91 `malloc`/`calloc`/
    `realloc` sites tree-wide to audit for the check-and-own rule.
  - **Leaked symbol / linkage inconsistency** — `src/game/computer_utilities.c:236`
    `int evasive_action[8][4]` is non-`static` (global symbol) while its sibling
    `agressive_action` at line 232 is correctly `static`; neither is declared in a
    header. Give `evasive_action` internal linkage.

## Goal

Apply tier (b) of the C standard to the first-party tree **surgically and
behavior-preservingly**, and wire up tier (a) so the toolchain enforces the rest.
Fix the real bugs; adopt the safe idioms where they clarify without churn; opt
out, in-code with a one-line reason, everywhere the obvious transform would harm
the artifact. Prove behavior unchanged where a gate can, and state per-change
what went unverified where no gate exists (the tree is almost entirely untested —
the only CTest is `cmake -E true`, `CMakeLists.txt:108-118`).

## Plan (phased; each phase re-verifies before the next)

### Phase 0 — Land the verification net + C23 compatibility gate (do before any source edit)
Scope and standard are settled (see Decisions). Phase 0 is now purely sequencing:
- [x] **Sanitizer gate landed first** (decision 4, done 2026-10-17). `tasks/archive/2026/10/17/add-sanitizer-gate.md`
      (UBSan-trap + ASan under Xvfb/software GL, bounded run) so this sweep has a
      runtime net before Phase 2 starts mutating memory/string code. The sweep doc's
      order still holds underneath: raised-warning-level build is the first gate,
      UBSan/ASan the runtime net.
- [ ] **C23 compatibility gate (front-loaded because we pin C23 now, decision 2).**
      Before adopting any C23 idiom, pin `-std` to C23 (Phase 1) and get a clean build
      proving C23 mode doesn't break: vendored `third_party/HandmadeMath/HandmadeMath.h`
      (its `_Generic` shorthand), the SDL3 / SDL3_sound / GLEW / PNG / Lua 5.4 headers,
      and the `.cpp` units (`project(... C CXX)` — C23 pin is C-only; confirm the CXX
      standard is untouched). **Cross-platform risk is the live one:** macOS clang is
      fine, but **MSVC's C23 support is partial** — confirm the Windows toolchain we
      claim (`CLAUDE.md` Status) still builds under C23, or record that Windows is
      deferred/untested. If a header breaks only under strict C23, that is the one
      place `gnu23` (extensions already ON) may be required — note it.
- [ ] Add a Phase-0 back-pointer from `tasks/c23-port-investigation.md` to this task
      (decision 3: that task stays the C23 decision record; Phase 4 here is its
      execution arm). Keep inbound pointers honest per convention.

### Phase 1 — Tier (a): let the toolchain build the worklist (mechanical, no hand-edits yet)
- [ ] Pin `-std=c23` in `CMakeLists.txt` (`set(CMAKE_C_STANDARD 23)`; keep
      `CMAKE_C_EXTENSIONS ON` for `realpath()`, so the effective flag is `gnu23`) and
      **confirm the flag reaches the compile** (grep `compile_commands.json` — the
      sweep doc warns a flag probe can silently no-op). The CXX standard is separate
      and stays as-is.
- [ ] Raise the first-party warning level **by default** (decision 5): add
      `-Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wvla
      -Wstrict-prototypes -Wold-style-definition -Wwrite-strings -Wformat=2` to the
      `gltron_warnings` INTERFACE lib, which is applied to **first-party code only,
      never `nebu`'s vendored deps or `third_party/`**. Fold `-Wall -Werror` in so the
      gate is on without needing `GLTRON_WARN=ON` (keep a knob to turn it *off* for a
      quick local build if wanted, but default is on).
- [ ] Add a `lint` step (default in the image gate) that runs the already-configured
      `.clang-tidy` scoped via `-header-filter` to **our sources only — never
      `third_party/` or system headers** — and Cppcheck
      (`--enable=warning,style,performance,portability`). Capture both outputs as the
      worklist. Diff + build after any `-fix` run.
- [ ] **The output of Phase 1 is a categorized worklist** (class × count ×
      file:line), logged under `tasks/adhoc/c-modernization-sweep/data/`. Review it
      with Bill before Phase 2 — some warnings are preservation hazards to opt out
      of, not fix.

### Phase 2 — Fix the real safety bugs the tools flag
- [ ] **`sprintf` → `snprintf` with the real buffer size**, only where `dst` is a
      true in-scope array (so `sizeof dst` is valid); check the `>= size` truncation
      return. For the `malloc(PATH_MAX)` case track the known length. (`path.c`,
      `scripting_interface.c`, `graphics_hud.c`, `sound_glue.cpp`, `material.c`.)
      **Hazard:** do NOT convert a `strncpy` whose zero-fill is serialized to disk/
      wire (sweep doc) — audit each before touching; gltron's look like plain string
      builds, but confirm.
- [ ] **Allocation checks + stated ownership**: NULL-check each `malloc`/`calloc`,
      `realloc` into a temp, `free`-then-NULL where a double-free/UAF is reachable,
      and write the one-line owner contract on functions returning owned memory
      (`getPossiblePath` = "caller frees"). Keep the game's by-design exit-time leaks
      (tracked in the sanitizer gate's LSan-off decision) — don't chase those.
- [ ] **`static` on `evasive_action`** (`computer_utilities.c:236`).

### Phase 3 — Safe idioms, class-by-class, surgical (re-verify after each class)
- [ ] **`bool`/`true`/`false`** (C23 keywords — no `<stdbool.h>` needed) for variables
      and returns that are truth values (not counts, not the `int` menu/action codes
      that are genuinely small ints).
- [ ] **Narrow `const`**: input-pointer params as `const T *`, and locals never
      reassigned. **Opt out** of the sweep-doc hazards: address-taken / output-filled
      locals (`&x` into `fstat`/`getPositionFromIndex`-style out-params),
      loop-carried accumulators, any `setjmp`/`longjmp` locals (grep first).
- [ ] **Fixed-width types ONLY where width matters** (file/wire/bitmask/register) —
      do **not** churn the `int i` loop counters or the `int dir`/`int player`
      indices that mirror the game's own small-int model; that is the pedagogy, not
      a smell (sweep doc: "correct domain types are not a smell").
- [ ] Trim the redundant `return; break;` in total `switch`es (`path.c:78-106`);
      keep the mandatory `default` (standard's total-dispatch rule) — gltron already
      writes defaults, good.
- [ ] Zero-init structs with `{ }`/`{ 0 }` (per standard pin) over `memset` where it
      reads clearer; do not reorder declarations purely to satisfy declare-at-use if
      it hurts the top-to-bottom read.

### Phase 4 — C23 idioms (ACTIVE — C23 is pinned, decision 2; this is the execution arm of c23-port-investigation, decision 3)
- [ ] `nullptr` for the 173 `NULL`s (catches pointer-vs-int mix-ups), standard
      attributes (`[[nodiscard]]` on allocators, `[[maybe_unused]]`, `[[fallthrough]]`),
      keyword `static_assert`. (Keyword `bool`/`true`/`false` already handled in
      Phase 3 — no `<stdbool.h>` to add.)
- [ ] All of this is **first-party only**. The C23 compatibility of vendored/system
      headers was already established in Phase 0 by *consuming* them under C23 — if one
      had broken, the fix is our pin/flags, **never** a patch to `third_party/` (see
      decision: no third-party source changes).

## Hard exclusion: no third-party / vendored source changes (decision, 2026-10-17)

**This sweep touches first-party code only — nothing under `third_party/`, and no
vendored single-header libraries.** Explicitly: **no HandmadeMath edits** (no
"fixes", no modernization, no reformatting), and the same for any other vendored or
system header. We only ever *consume* those headers; if one is unhappy under our C23
pin or raised warnings, the response is to adjust *our* flags / `-header-filter` /
suppressions at *our* call sites — never to patch the dependency. clang-tidy and the
warning level are both already scoped to first-party sources, which enforces this
mechanically. (`nebu/` is the in-tree first-party engine and IS in scope, but with a
stable public surface — see below.)

## Preservation hazards specific to gltron (do NOT "fix" these)

- **Fixed-function OpenGL** — untouched. That is the whole point of the fork; the
  core-profile port is its own deferred task (`tasks/opengl-core-profile-port.md`).
- **HandmadeMath** (vendored, header-only) — out of scope entirely per the exclusion
  above; consume it, don't touch it, and keep the C11-or-later pin its `_Generic`
  shorthand needs.
- **`assert(0)` as the error strategy** — this is gltron's chosen convention
  (`path.c:66,103`); the standard asks for *one* strategy stated in CLAUDE.md, not
  that we replace this one. Record it in CLAUDE.md rather than rewriting call sites.
- **Small-int domain model** (`dir`, `player`, menu/action codes) — intentionally
  plain `int`; not a `size_t`/fixed-width candidate.
- **The nebu engine boundary** — nebu is a separate in-tree library; sweep it on the
  same rules but keep its public surface stable (no signature churn on `Nebu_*`).
- Any teaching comment / deliberate simplification — preserve behavior and the lesson.

## Verification

- **Primary gate:** clean build at the raised warning level (Phase 1 flags), plus
  `.clang-tidy`/Cppcheck clean on first-party sources. This catches the bulk
  (uninitialized reads, pointer-decay `sizeof`, narrowing) per the sweep doc.
- **Runtime net:** the UBSan-trap + ASan gate from `tasks/archive/2026/10/17/add-sanitizer-gate.md`,
  run under Xvfb + software GL (that task already worked out the display problem and
  the bounded-run hook). If that gate is not landed first (open q 4), the sweep runs
  without a runtime net and **every change states what it could not verify** — do not
  claim a runtime guarantee the (empty) test suite didn't provide.
- Run the whole thing in the podman image (`make -f Makefile.docker image` / `build`),
  per the repo's in-container arrangement.

## Relationship to the two adjacent tasks

- **`tasks/c23-port-investigation.md`** (Priority 7) stays the **C23 decision record**;
  **Phase 4 here is its execution arm** (decision 3). Add a back-pointer from it to
  this task in Phase 0; don't archive/delete it without go-ahead.
- **`tasks/archive/2026/10/17/add-sanitizer-gate.md`** was the verification mechanism and **landed first** (done 2026-10-17)
  (decision 4) — this sweep is its best justification (it gives the gate real bugs to
  catch).

## Decisions (settled 2026-10-17, William Emerison Six <billsix@gmail.com>)

1. **Scope** = the **safety/correctness + safe-idioms surgical sweep** (Phases 1–4 as
   written): fix the `sprintf`/alloc bugs, add `bool`/narrow `const`/`static`, adopt
   the C23 idioms, turn on the warning + lint gate. **Not** a tree-wide
   declare-at-use / reflow rewrite — that would risk the read-top-to-bottom pedagogy
   and the no-drive-by rule.
2. **Standard pin = C23 now** (`set(CMAKE_C_STANDARD 23)`, extensions on → `gnu23`).
   Image clang 22 / gcc 16 are fine; the cross-platform risk (notably MSVC's partial
   C23 support) is accepted and handled by the front-loaded **C23 compatibility gate**
   in Phase 0 — if Windows can't build under C23, record it as deferred/untested
   rather than blocking the sweep.
3. **Keep `c23-port-investigation.md` separate** as the C23 decision doc; Phase 4 is
   its execution. Cross-link both ways (Phase 0).
4. **Sanitizer gate first** — **done** (landed 2026-10-17); the net is in place before Phase 2 mutates any
   memory/string code.
5. **Warning + lint gate on by default** — the raised `-W` flags and the clang-tidy/
   Cppcheck `lint` step are the default image gate (not gated behind an off-by-default
   `GLTRON_WARN`), so CI actually enforces tier (a).
6. **No third-party / vendored source changes** (added per the maintainer, 2026-10-17)
   — first-party only; explicitly no HandmadeMath edits. See the Hard exclusion
   section above.
