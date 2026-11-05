# Finish top-level `const` on pointers: the 4 file-scope tables and the 37 missed locals

**Status:** proposed — needs go-ahead (open questions 1 and 2)
**Priority:** 6
**Difficulty:** 2
**Created:** 2026-11-05 (William Emerison Six <billsix@gmail.com>)

## BLUF

The maintainer asked whether gltron's C uses `const` on both levels of a pointer (`const char *const name`).
An audit on 2026-11-05 found commit `63ceb1ee` ("pointer const-correctness sweep (pointee + top-level)") did
it for locals, but missed 37. That sweep skipped parameters on purpose, and never touched the 4 file-scope
pointer tables, the one case where the second `const` has a runtime benefit. Done means the 4 tables and the
37 locals are `T *const`, the build is clean at the `-Werror` gate under gcc and clang, and the sanitizer gate
passes.

## Context (read first)

- The shared C standard's "`const` by default" bullet, in the c-coding-standard.md reference doc of
  github.com/billsix/runClaudeInContainer (mirrored in runCrushInContainer). It was updated 2026-11-05 to
  cover file-scope tables and to say `clang-tidy misc-const-correctness` is C++-only.
- `tasks/archive/2026/10/17/c-modernization-sweep.md` documents only the pointee-const `-Wwrite-strings`
  phase; the two-level sweep is commit `63ceb1ee` (63 files, about 70 `* const`), whose message says
  "never-repointed locals (not parameters)". The setjmp/longjmp hazard (libpng's error path) is in the
  modernization-sweep reference doc.
- gltron's "no drive-by modernization" rule (its `CLAUDE.md`) is why parameters and late-assigned locals are
  out of scope by default.

## Audit method (2026-11-05; reproducible)

- `clang-tidy misc-const-correctness` with `WarnPointersAsValues: true` reports **nothing on C** (LLVM 22.1.8;
  it works on the `.cpp` units only). So a libclang (python3-clang) AST script was used instead. It ran over a
  scratch CMake build's `compile_commands.json` (`cmake -S . -B <scratch> -G Ninja`, src/ and nebu/; the only
  thing in `third_party/` is HandmadeMath, and it was skipped). For every pointer binding the script counted
  real writes to the pointer itself: `=`, compound assignment, `++`/`--`, and `&p`. Writes *through* the
  pointer (`*p =`, `p[i] =`) don't count.
- **Compile check:** adding `const` to all 389 strict candidates in a `git archive` copy built clean at
  `-Werror` under clang 22 and gcc 16.
- **Codegen check:** comparing Release `.text` sections, 72 of 75 objects were byte-identical. The 3 that
  differed (credits.c, switchCallbacks.c, video.c) read the newly const tables. So locals and params are
  codegen-neutral, and tables let the compiler assume immutability.

## Counts

| Category | src/ | nebu/ | In scope? |
|---|---|---|---|
| File-scope pointer tables | 4 | 0 | **yes** |
| Locals initialized once and never written (missed by `63ceb1ee`) | 35 | 2 | **yes** |
| Locals declared, then assigned exactly once (need the declaration moved to the assignment) | 41 | 9 | open question 1 |
| Parameters (definitions) never reassigned | 201 | 158 | no (house rule; see open question 2) |

## The work

**Tables** (each moves from `.data` to `.data.rel.ro`):

- `src/audio/sound.c` `static const char *game_fx_names[]`
- `src/game/credits.c` `const char *credits[]`
- `src/game/globals.c` `const char *lc_lod_names[]`, together with its `extern` in `src/include/video/video.h`
  (the qualifiers must match)
- `src/game/switchCallbacks.c` `Callbacks *callbackList[N_CALLBACKS]` → `Callbacks *const callbackList[]`

`credits` and `callbackList` have no `extern` anywhere, so they could also become `static`. That is a separate
one-word change each, and fine to fold in.

**Missed locals.** Examples (re-run the AST script for the full list of 37):

- `computer_utilities.c:ai_getDistances` `float *front = &distances->front;`, and `right`, `left`,
  `backleft`
- `computer.c:doComputerActive` `Data *data`; `event.c:applyBooster` / `applyWallBuster` / `doMovement`
  `Data *data`
- `model.c:gltron_Mesh_LoadFromFile` `pVertices`, `pNormals`, `pMesh`
- `event.c:Game_Idle` `nebu_List *l = p;`; `video.c:initVideoData` `Visual *d = gScreen`
- `screenshot.c:doPngScreenShot` `char *path`; `resource.c:resource_Get` `Token *pToken`

**Do not touch:**

- loop and list cursors;
- reassigned parameters (`segment2_Intersect` `pOut`, `goto_installpath` `argv0`, `getGlobal` `s`);
- out-params whose address is passed: `scripting_GetStringResult(&artpack)` callers, `strtol(&endptr)`, and
  libpng's `png_destroy_*_struct(&png_ptr, &info_ptr)`, whose setjmp-live locals must be left alone anyway;
- tables written at run time (`sound_glue.cpp` `players[]`, `scripting_interface.c` `art_dirs[]`);
- mutable globals assigned in other translation units.

## Plan

1. Recreate the AST finder: a small libclang script, idempotent. Because a later gate might use it, save it
   as `tasks/adhoc/toplevel-const-pointers/find_toplevel_const.py`, plus its output snapshot under `data/`.
2. Apply the 4 tables and the `extern`, then the 37 locals, matching on content rather than line numbers.
3. Gates: build at the `-Werror` gate under gcc and clang; `make image` (nested) with the sanitizer gate;
   `make format`; play-test that the credits and the sound effects still work.
4. Per open question 3, either promote the finder to `tools/` or `git rm` it at archive time.

## Open questions

1. Also do the 50 "declared, then assigned once" locals? This means moving each declaration down to its
   assignment, which is restructuring rather than adding a qualifier. Recommendation: no, except where the
   move is a trivial adjacent-line merge. gltron's churn rule argues against the rest.
2. Parameters (359): skip, per the shared standard (top-level `const` on a parameter is a definition-only
   detail that clutters signatures) and the earlier sweep's explicit choice? Recommendation: skip.
3. Promote the finder to `tools/check_toplevel_const.py` as an advisory check (later made fatal by
   `tasks/lint-fatal-gate.md`), or delete it after this task? Recommendation: promote it to `tools/` but wire
   it in only when the lint-fatal-gate task runs, since clang-tidy can't cover C here.
