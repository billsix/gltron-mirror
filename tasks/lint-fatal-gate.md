# Promote static analysis (clang-tidy + Cppcheck) to a fatal gate

**Status:** proposed — needs go-ahead
**Priority:** 8
**Difficulty:** 7
**Created:** 2026-10-17 (William Emerison Six <billsix@gmail.com>)
**Depends on / follows:** `tasks/archive/2026/10/17/c-modernization-sweep.md` (the sweep that
made the *compiler* warning level a `-Werror` gate and left static analysis advisory).

## BLUF

Drive the first-party clang-tidy **bug-finding** findings (~580: `bugprone-*`,
`clang-analyzer-*`, `cert-*`) and the Cppcheck findings (~1139) down to zero-or-opted-out,
then **promote `tools/lint.sh` from advisory (always exits 0) to a fatal gate** — the
static-analysis counterpart to the compiler `-Werror` gate the modernization sweep already
enforces. "Done" = `tools/lint.sh` exits non-zero on any new finding, the first-party tree is
clean or every residual finding is opted out in-code with a one-line reason, the lint gate runs
in the image build (clang + gcc where applicable), and **no gltron behavior or teaching value
changed** (same contract as the sweep). This is explicitly **not** a license to churn the
artifact: several check classes will be *disabled with rationale* rather than "fixed".

## Context (read first)

1. **`tools/lint.sh`** — the advisory script the sweep added. Currently runs clang-tidy
   (`-*,bugprone-*,clang-analyzer-*,cert-*`) + Cppcheck over first-party sources and exits 0.
   The project `.clang-tidy` (`bugprone-*,clang-analyzer-*,readability-*,performance-*,
   portability-*,cert-*,misc-*,concurrency-*` with several `-` exclusions) has
   `WarningsAsErrors: ''` (nothing fatal).
2. **The advisory snapshot** — `tasks/adhoc/c-modernization-sweep/data/lint-advisory-snapshot.txt`
   (captured 2026-10-17): the full current findings. Top classes:
   - clang-tidy: `bugprone-narrowing-conversions` 197, `bugprone-macro-parentheses` 179,
     `cert-err33-c` (unchecked return) 121, `bugprone-implicit-widening-of-multiplication-result`
     34, `cert-int09-c` 13, `bugprone-switch-missing-default-case` 13, …
   - Cppcheck: ~1139 (e.g. `variableScope`, `uninitMemberVarNoCtor`, style/performance).
3. **Why this is a separate, go-ahead-gated task.** On a 1999 teaching artifact these are
   overwhelmingly legacy/teaching patterns, not live bugs: deliberately simple macros
   (`macro-parentheses`), the small-int domain model (`narrowing-conversions`), and the classic
   un-checked `printf`/`scanf` style (`cert-err33-c`). Blindly "fixing" all ~1700 would be exactly
   the drive-by churn gltron's `CLAUDE.md` forbids. So the work is **per-check policy first**, like
   the sweep's warning-flag triage — decide fix / opt-out / disable-the-check, then execute.
4. **Method & constraints** — reuse the sweep's playbook: the C coding standard and
   `c-modernization-sweep.md` reference docs (canonical in `github.com/billsix/runClaudeInContainer`,
   mounted at `~/.claude/reference/`), behavior-preservation, fresh per-directory Edit-only
   subagents for the safe bulk, the UBSan+ASan gate as the runtime net, and the in-image gate as
   the authoritative check. gltron is a teaching artifact; fixed-function OpenGL and the
   read-top-to-bottom shape stay.

## Open question (blocks the work)

1. **Is a fatal clang-tidy/Cppcheck gate actually wanted for this artifact, and at what check
   scope?** Three honest options — pick one (or a per-check mix):
   (a) **Narrow fatal gate** *(recommended)* — make fatal only the checks that catch real defects
   and are near-zero already or cheaply fixable (e.g. `clang-analyzer-*`, `bugprone-sizeof-*`,
   `bugprone-suspicious-*`, `cert-err33-c` **if** we adopt an explicit `(void)` discard idiom),
   and **disable** the churny legacy classes in `.clang-tidy` with a rationale comment
   (`bugprone-narrowing-conversions`, `bugprone-macro-parentheses`, most Cppcheck `style`). Smallest
   churn, real safety value, honestly enforceable.
   (b) **Full fatal gate** — drive *every* listed check to zero or in-code opt-out (~1700 sites).
   Maximal, but large churn on deliberately-old code; risks the teaching value.
   (c) **Stay advisory** — keep `lint.sh` exit 0; no gate. (Then this task closes as "won't do".)

## Plan (phased; settle the open question before Phase 2)

### Phase 0 — refresh the worklist
- [ ] Re-run `tools/lint.sh` and snapshot findings to `tasks/adhoc/lint-fatal-gate/data/`
      (class × count × file:line), since the tree may have moved since 2026-10-17.

### Phase 1 — per-check policy (the real decision)
- [ ] Classify every check class: **fix** (real defect, cheap), **opt-out** (correct-as-is, mark
      `// NOLINT(check)` / `/* cppcheck-suppress id */` with reason), or **disable** (legacy/teaching
      pattern, turn off in `.clang-tidy` / Cppcheck with a rationale comment). Review with the
      maintainer — this is the go-ahead gate, like the sweep's warning triage.

### Phase 2 — execute the safe bulk (behavior-preserving)
- [ ] Apply the fixes class-by-class (fresh per-directory Edit-only subagents for the mechanical
      bulk; verify each diff + rebuild + sanitizer gate), opting out the hazards in-code.
- [ ] Add the agreed `.clang-tidy` disables and narrow suppressions.

### Phase 3 — flip the gate
- [ ] Set `.clang-tidy` `WarningsAsErrors` to the kept checks (or have `lint.sh` exit non-zero on
      any finding in the kept set), and make `tools/lint.sh` fatal.
- [ ] Wire it into the image gate (`Dockerfile`/`Makefile.docker`) alongside the sanitizer gate,
      default on, with a knob to skip.

### Phase 4 — verify
- [ ] `tools/lint.sh` clean (or all-opted-out) for first-party; both compilers still build at
      `-Werror`; UBSan+ASan timedemo gate ALL PASS; authoritative `make image` green.

## Verification
- Same nets as the sweep: raised-warning `-Werror` build (clang + gcc), UBSan-trap + ASan timedemo,
  run in the podman image. The new gate is "`tools/lint.sh` exits 0 with the kept checks fatal."

## Notes
- Cross-link: this is the static-analysis counterpart to the compiler `-Werror` gate landed by the
  modernization sweep; the sweep's Phase 1 deliberately left this layer advisory (see its
  "advisory lint tool" commit and `CLAUDE.md` "Static analysis is advisory, not a gate").
