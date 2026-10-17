# Behavior-preserving C modernization sweep

**Status:** proposed — needs go-ahead
**Priority:** 6
**Difficulty:** 5
**Created:** 2026-10-17 (William Emerison Six <billsix@gmail.com>)

## BLUF

Sweep the ~14 KLOC of first-party C (`src/`, `nebu/`) toward the maintainer's
shared C coding standard **class-by-class and behavior-preservingly** — fixing
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
4. `tasks/c23-port-investigation.md` and `tasks/add-sanitizer-gate.md` — the two
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

### Phase 0 — Decisions + the verification net (do before any edit)
- [ ] Settle the standard pin (open q 2) and the scope line (open q 1).
- [ ] Settle whether the **sanitizer gate** (`tasks/add-sanitizer-gate.md`) lands
      first as this sweep's verification net, or we sweep with build+clang-tidy
      only (open q 4). The sweep doc is explicit that for an untested tree the
      *build at a raised warning level* is the first gate and UBSan/ASan is the
      runtime net; without a gate, each change is "mechanical + clang-tidy-clean +
      stated-unverified."

### Phase 1 — Tier (a): let the toolchain build the worklist (mechanical, no hand-edits yet)
- [ ] Pin `-std` explicitly (per open q 2) and confirm the flag reaches the compile
      (grep `compile_commands.json` — the sweep doc warns a flag probe can silently
      no-op). Keep `CMAKE_C_EXTENSIONS ON` (realpath).
- [ ] Raise the first-party warning level: add `-Wextra -Wpedantic -Wconversion
      -Wsign-conversion -Wshadow -Wvla -Wstrict-prototypes -Wold-style-definition
      -Wwrite-strings -Wformat=2` to the `gltron_warnings` INTERFACE lib (applied
      to first-party code only, never vendored). Decide whether this becomes the
      default or stays under `GLTRON_WARN` (open q 5).
- [ ] Add a `lint` step that runs the already-configured `.clang-tidy` (scoped via
      `-header-filter` to our sources, never `third_party/`/system headers) and
      Cppcheck (`--enable=warning,style,performance,portability`). Capture both
      outputs as the worklist. Diff + build after any `-fix` run.
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
      (tracked in `add-sanitizer-gate.md`'s LSan-off decision) — don't chase those.
- [ ] **`static` on `evasive_action`** (`computer_utilities.c:236`).

### Phase 3 — Safe idioms, class-by-class, surgical (re-verify after each class)
- [ ] **`bool`/`true`/`false`** for variables and returns that are truth values
      (not counts, not the `int` menu/action codes that are genuinely small ints).
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

### Phase 4 — C23 niceties (OPTIONAL; gated on open q 2/3 — this is where c23-port-investigation folds in)
- [ ] Only if we pin C23: `nullptr` for the 173 `NULL`s (catches pointer-vs-int
      mix-ups), standard attributes (`[[nodiscard]]` on allocators, `[[maybe_unused]]`,
      `[[fallthrough]]`), keyword `bool`/`static_assert` (drop `<stdbool.h>` once
      added). **Compatibility check** (from c23-investigation): verify C23 mode
      doesn't break the vendored `third_party/HandmadeMath/HandmadeMath.h` `_Generic`
      path, SDL3 / GLEW / Lua 5.4 headers, and the Windows/macOS toolchains we claim.

## Preservation hazards specific to gltron (do NOT "fix" these)

- **Fixed-function OpenGL** — untouched. That is the whole point of the fork; the
  core-profile port is its own deferred task (`tasks/opengl-core-profile-port.md`).
- **HandmadeMath `_Generic`** (vendored, header-only, C11) — don't modernize vendored
  code, and don't drop the C11-or-later pin it needs.
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
- **Runtime net:** the UBSan-trap + ASan gate from `tasks/add-sanitizer-gate.md`,
  run under Xvfb + software GL (that task already worked out the display problem and
  the bounded-run hook). If that gate is not landed first (open q 4), the sweep runs
  without a runtime net and **every change states what it could not verify** — do not
  claim a runtime guarantee the (empty) test suite didn't provide.
- Run the whole thing in the podman image (`make -f Makefile.docker image` / `build`),
  per the repo's in-container arrangement.

## Relationship to the two adjacent tasks

- **`tasks/c23-port-investigation.md`** (Priority 7) overlaps Phase 4 and the feature
  survey. Recommendation: keep it as the C23-decision record and let Phase 4 *be* its
  execution, or fold it into this task — see open q 3. Don't archive/delete it without
  go-ahead.
- **`tasks/add-sanitizer-gate.md`** is the verification mechanism; this sweep is its
  best justification (it gives the gate real bugs to catch). See open q 4.

## Open questions

1. **Scope line.** Recommended: a **safety/correctness + safe-idioms surgical sweep**
   (Phases 1–3 as written) — fix the `sprintf`/alloc bugs, add `bool`/narrow `const`/
   `static`, turn on the warning gate; explicitly *not* a tree-wide declare-at-use /
   reflow rewrite (that risks the read-top-to-bottom pedagogy and the no-drive-by
   rule). Do you want exactly this scope, or narrower (bugs-only, Phase 2) / broader
   (include the full stylistic sweep)?
2. **Standard pin.** Recommended: pin **C17** for the sweep (well-supported on the
   Windows/macOS toolchains the repo claims; removes the "unpinned → whatever the
   compiler defaults to" nondeterminism) and treat **C23** idioms (`nullptr`,
   attributes, keyword `bool`) as the optional Phase 4 behind a compatibility check.
   Or: pin C23 now (image clang 22 / gcc 16 are fine) and accept the cross-platform
   risk? One answer: C17-now / C23-now.
3. **Fold or keep `c23-port-investigation.md`?** Recommended: keep it as the C23
   decision doc and make Phase 4 its execution arm (cross-linked). Or should I fold
   its content into this task and archive it? (a) keep separate, or (b) fold in.
4. **Sanitizer gate first?** Recommended: land `add-sanitizer-gate.md` first so the
   sweep has a runtime net. Or sweep now with build+clang-tidy only and add the gate
   after? (a) gate-first, or (b) sweep-first.
5. **Default the warning level?** Recommended: make the raised flags (and a `lint`
   step) the default gate rather than leaving them under the off-by-default
   `GLTRON_WARN`, so CI actually enforces tier (a). Or keep them opt-in? (a) default
   on, or (b) keep opt-in.
