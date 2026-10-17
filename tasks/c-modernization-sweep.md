# Behavior-preserving C modernization sweep — DONE

**Status:** done 2026-10-17 (William Emerison Six <billsix@gmail.com>)
**Priority:** 6 · **Difficulty:** 5

## BLUF

Swept the first-party C (`src/`, `nebu/`) toward the maintainer's shared C coding standard
**class-by-class and behavior-preservingly**: fixed the real safety bugs, adopted the safe
idioms, pinned **C23**, and turned the first-party warning level into an enforced **`-Werror`
gate** (clean under clang 22 **and** gcc 16). First-party warnings went **493 → 0**. Nothing
under `third_party/` or HandmadeMath was touched; the fixed-function-OpenGL pedagogy and the
read-top-to-bottom shape are unchanged. Verified by the raised-warning build (both compilers),
the UBSan-trap + ASan timedemo gate (ALL PASS), and the authoritative in-image
`make image RUN_SANITIZERS=1` (green; image `a1da21cca8a1`).

## Result

- **Warnings:** 493 → 333 (enumerated gate flags, int→float + unused-parameter dropped) → 327
  (safety fixes) → 231 → 174 (const) → 5 → **0**. `-Werror` on by default.
- **C23 pinned** (`CMAKE_C_STANDARD 23`, extensions on → `gnu23`); `nullptr`/attributes adopted.
- **Both compilers clean** at the full gate; **sanitizer gate ALL PASS** at every mutating phase
  and in-image.
- **No `third_party/` / HandmadeMath edits**; the dormant `*-macos.c`/`*-win32.c` variants (not in
  the Linux build) were left untouched.

## What was done (by phase; past tense)

**Phase 0 — nets & compat.** Pinned `-std=c23` and proved C23 mode builds clean (HandmadeMath
`_Generic`, SDL3/SDL3_sound/GLEW/PNG/Lua headers, the `.cpp` units); flag confirmed on 67 TUs in
`compile_commands.json`. Windows/MSVC C23 left deferred/untested (recorded, not a blocker). The
sanitizer gate (`tasks/archive/2026/10/17/add-sanitizer-gate.md`) was already the runtime net.

**Phase 1 — the warning gate.** Wired `gltron_warnings` with the enumerated raised set
(`-Wall -Wextra -Wpedantic -Wshadow -Wvla -Wwrite-strings -Wformat=2 -Wsign-conversion`, plus
C-only `-Wstrict-prototypes`/`-Wold-style-definition` and clang-only `-Wshorten-64-to-32`/
`-Wimplicit-int-conversion`), on by default, scoped to first-party only. Deliberately **not** the
`-Wconversion` umbrella (drops int→float) and `-Wno-unused-parameter` (callback signatures).
Vendored/system includes made `SYSTEM` so they never trip the gate (gcc `-Wpedantic` flags ~140
GNU-extension uses in HandmadeMath.h). `-Werror` flipped on once the tree was clean. Worklist
captured under `tasks/adhoc/c-modernization-sweep/` (`discover.sh` reproduces it).

**Phase 2 — real safety bugs.** `dirsetup.h` header-guard typo (`#define DIRESTUP_H`; the guard
never armed) fixed; unbounded `sprintf`→`snprintf` with the real buffer size across `path.c`,
`scripting_interface.c`, `graphics_hud.c` (gave the file-local `getPauseString` a `bufsize` param),
`gui.c`, `screenshot.c`, `video.c`, `console.c`, `sound_glue.cpp`; non-literal format strings closed
with `"%s"`; `PREF_DIR+1`→`&PREF_DIR[1]`; `getPossiblePath` malloc NULL-checked with its owner
contract stated; `evasive_action` given internal linkage. Exactly-sized `malloc(strlen+1)`+`strcpy`
left as-is (safe). The sound-source name buffer length named once (`kSoundSourceNameLen`) — see the
idiom added to the shared C standard.

**Phase 3 — safe idioms (fanned out, behavior-preserving).** 153 `-Wwrite-strings` cleared by
`const char*` on read-only string tables/fields/params (the `keyname.name` field alone cleared 96).
130 `-Wsign-conversion` fixed as **boundary casts** (never retyping arithmetic-bearing
members/locals), plus `-Wshadow` inner renames, `-Wpointer-sign` GL casts, narrowing casts, dead-code
removal / `[[maybe_unused]]`, and `[[gnu::format]]` on the two printf-wrappers. Done via fresh
per-directory Edit-only subagents under a strict cast-at-boundary rule; every diff reviewed, rebuilt,
and run through the sanitizer gate (ALL PASS). Then clang-formatted.

**Phase 4 — C23 idioms.** `NULL`→`nullptr` across the compiled first-party tree (clang-tidy
`modernize-use-nullptr` for pointer-literals, then a guarded sweep for the comparison/variadic cases;
no `NULL` in any string literal, so behavior-identical; the clean build proves no `NULL`-as-int
misuse existed). `[[nodiscard]]` on `getPossiblePath`; `[[fallthrough]]` on the one intentional
case-0 fall-through. Execution arm of `tasks/c23-port-investigation.md`.

**Phase 1 close — gcc parity + lint.** Fixed the 7 first-party sites gcc flags that clang does not
(a sign-conversion cast, three typedef-shadowing locals renamed, two int→GLenum casts, a
`const static`→`static const`). Added **advisory** `tools/lint.sh` (clang-tidy bug-finding checks +
Cppcheck, scoped, always exits 0) — **not** a fatal gate, because the project `.clang-tidy` yields
~580 clang-tidy + ~1139 Cppcheck findings of legacy/teaching style on this artifact. Documented the
gate and gltron's error strategy in `CLAUDE.md`.

## Decisions (settled 2026-10-17, William Emerison Six <billsix@gmail.com>)

1. Scope = safety/correctness + safe-idiom surgical sweep (not a tree-wide reflow/declare-at-use
   rewrite). 2. **Pin C23 now.** 3. Keep `c23-port-investigation.md` separate; Phase 4 is its
   execution arm (cross-linked). 4. Sanitizer gate landed first (the runtime net). 5. Warning + lint
   gate on by default — the **compiler `-Werror`** level is the enforced tier-(a) gate; the
   clang-tidy/Cppcheck lint step is **advisory, not fatal** (revised from "fatal" on contact with
   reality — a fatal legacy-lint gate would force the churn gltron forbids; promotion tracked in
   `tasks/lint-fatal-gate.md`). 6. No third-party/vendored source changes — first-party only,
   explicitly no HandmadeMath edits (suppress at our consumption point via `SYSTEM` includes).

**Worklist triage (settled):** keep `-Wwrite-strings` (const the tables); keep `-Wsign-conversion`/
`-Wshorten-64-to-32` (fix real boundaries), **drop** `-Wimplicit-int-float-conversion` (intentional
int-coord→float math); `-Wno-unused-parameter` (externally-dictated callbacks); end state `-Werror`.

## Findings & follow-ups (preserved behavior-identically; NOT fixed here)

- **Latent bug (optional)** — `nebu/scripting/scripting.c`: computes a clamped length `copy` but the
  `memcpy` uses the unclamped `size`; the clamp is dead. Kept `[[maybe_unused]]`.
- **Latent bug (optional)** — `src/game/event.c` `crashTestPlayers` always returns 0 (inner `crash`,
  renamed `jCrash`, drives the break; the function returns the never-set outer `crash`).
- **Optional header retypes** — `video_level_shader.diffuse_texture_id`, `nebu_2d::tex_id`,
  `nebu_Font::pTextures`, and `trailStatesNormal(..., int texture)` hold GL ids as `int`; retyping to
  `GLuint`/`GLuint*` would drop ~8 call-site casts (deliberate cross-dir header change, not done here).
- **Fatal lint gate (recommended follow-up)** — `tasks/lint-fatal-gate.md`: promote `tools/lint.sh`
  from advisory to an enforced gate, the static-analysis counterpart to the compiler `-Werror` gate.

## Preservation hazards honored

Fixed-function OpenGL untouched; HandmadeMath consumed, not edited; `assert(0)` error strategy kept
(recorded in `CLAUDE.md`, not rewritten); small-int domain model (`dir`/`player`/menu codes) left as
plain `int`; nebu's public `Nebu_*` surface stable; teaching comments/simplifications preserved.

## Verification

- Raised-warning build clean at `-Werror` under **clang 22 and gcc 16**; 0 first-party warnings.
- **UBSan-trap + ASan** timedemo gate (`tools/sanitizer-gate.sh`, Xvfb + software GL, bounded run):
  ALL PASS at Phases 3 and 4 and in-image.
- Authoritative **`make -f Makefile.docker image RUN_SANITIZERS=1`**: green — gcc `-Werror` build,
  ctest 100% (1/1), both sanitizer lanes PASS; image `a1da21cca8a1`.
- clang-tidy/Cppcheck advisory snapshot in `tasks/adhoc/c-modernization-sweep/data/`.

## Relationship & artifacts

- **`tasks/c23-port-investigation.md`** — the C23 decision record; Phase 4 here was its execution arm.
- **`tasks/archive/2026/10/17/add-sanitizer-gate.md`** — the runtime net this sweep gave real bugs to catch.
- **`tasks/lint-fatal-gate.md`** — the follow-up to make static analysis a fatal gate.
- Gate wiring in `CMakeLists.txt` (`gltron_warnings`, `GLTRON_WARN`/`GLTRON_WERROR`, `SYSTEM` includes);
  `tools/lint.sh`; method reused from the shared `c-coding-standard.md` / `c-modernization-sweep.md`
  reference docs (canonical in `github.com/billsix/runClaudeInContainer`). The buffer-size-named-once
  idiom was added to that C standard (synced to runCrushInContainer's two copies; staged there).
