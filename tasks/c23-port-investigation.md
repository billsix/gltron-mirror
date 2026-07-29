# Investigate porting gltron from C11 to C23

**Status:** proposed — not started
**Created:** 2026-06-13

## Goal

Investigate what moving gltron from its current **C11** baseline to **C23** would
provide, decide which modern C features would actually benefit *this* codebase
(and which would just be churn), and produce a port plan with a clear
recommendation on whether it's worth doing at all.

Constraint to keep front-of-mind: gltron is an **intentional teaching artifact**
(fixed-function OpenGL preserved on purpose). Any C23 adoption has to be justified
by real readability/safety value and stay surgical — it must not obscure the
pedagogy or trigger a drive-by "modernization" of code that's deliberately old.

## Plan

- [ ] **Baseline & toolchain.** Confirm the current standard
      (`set(CMAKE_C_STANDARD 11)` in `CMakeLists.txt`), and check C23 support in
      the build toolchains: clang/gcc in the Fedora-44 image, plus any platform we
      claim to support (Windows, macOS). Note what `-std=c23`/`gnu23` requires and
      where support still lags (MSVC, older clang).
- [ ] **Feature survey → concrete use sites.** Go through the C23 additions and
      map each to actual places in `src/` and `nebu/` where it would help, e.g.:
      `nullptr`; attributes (`[[maybe_unused]]`, `[[fallthrough]]`, `[[nodiscard]]`,
      `[[deprecated]]`); `static_assert`/`bool`/`true`/`false` as keywords (drop
      `<stdbool.h>`/`<assert.h>` boilerplate); `constexpr` objects; `typeof`;
      enums with a fixed underlying type; `unreachable()`; binary literals + digit
      separators; `#embed` (could it inline shaders/assets, or do we keep runtime
      loading?); `_BitInt`; `memset_explicit`.
- [ ] **Triage value vs. churn.** For a teaching codebase, weight readability and
      safety (likely wins: `nullptr`, attributes, keyword `bool`/`static_assert`)
      against features that add cognitive load with little payoff here. Recommend a
      *minimal* adoption set vs. a *broad* one.
- [ ] **Compatibility check.** Verify C23 mode doesn't break inclusion of the
      vendored `third_party/HandmadeMath/HandmadeMath.h` or the SDL3 / Lua 5.4 /
      GLEW headers; check the `-Wall -Werror` path (GLTRON_WARN) under C23.
- [ ] **Pedagogy check.** Decide whether any C23 idiom undercuts the
      "read it top-to-bottom, fixed-function" teaching intent. Scope accordingly.
- [ ] **Write the plan.** Produce a phased port plan (per gltron's convention, as
      `tasks/c23-port.md`) with steps, risk, the recommended feature set, and
      an explicit "is this worth doing?" recommendation. Add it to the
      `## Tasks / plans (in-flight)` index in `CLAUDE.md` once it exists.

## Notes / decisions

- Current standard is C11, bumped from C99 by the HandmadeMath port (2026-05-08).
- This `tasks/` item tracks the *investigation*; the durable output (a port plan)
  belongs in `tasks/` per gltron's convention.

## Open questions

- Primary goal: pedagogical clarity, or modernization/safety? (Drives which
  features to adopt.)
- What's the minimum compiler baseline we must keep building on? (C23 support
  varies a lot — Fedora image clang is fine, but Windows/macOS toolchains may lag.)
- `#embed` for shaders/assets, or keep them runtime-loaded as today?
