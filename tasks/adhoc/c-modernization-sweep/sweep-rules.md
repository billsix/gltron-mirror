# Phase-3 sweep rules (for the per-directory edit workers)

You are fixing compiler warnings in **one directory** of the gltron tree as part of a
behavior-preserving C modernization sweep. gltron is a **1999 teaching artifact**:
behavior and the read-top-to-bottom shape must not change. Your job is to make the
listed warnings go away with the **smallest behavior-identical edit**, or to opt out
in place (with a one-line `// ` reason) where the fix would be wrong.

**The tree is almost untested** — do not change observable behavior. When a fix is not
clearly behavior-identical, prefer a boundary cast or opt-out over a risky retype.

## Hard limits (do NOT do these)

- **Edit only.** Do NOT run git, cmake, ninja, make, clang-format, or any shell build.
  Only edit source files with your editing tools.
- Touch **only files in your assigned directory**. Headers your directory's `.c`/`.cpp`
  include may live elsewhere; if a fix needs a struct/prototype change in a header outside
  your directory, DO NOT edit it — instead note it in your report as "needs header change:
  <file> <what>" and cast/opt-out locally so your file still builds.
- Never touch `third_party/` or `HandmadeMath` (vendored), and never `*-macos.c` /
  `*-win32.c` (not in the Linux build).
- Do not change the warning flags, CMakeLists, or any `.cmake`.
- No drive-by cleanup: no reflow, no reordering declarations, no renames except a `-Wshadow`
  fix, no "modernization" beyond the specific warning on the specific line.

## Per-warning-class fix patterns

- **`-Wincompatible-pointer-types-discards-qualifiers` / `-Wwrite-strings`** — a `char*`
  holds/receives a string literal. Make it `const char*`:
  - static/local array of string literals → `const char* x[] = {...}`.
  - a function **parameter** that receives a literal → make the parameter `const char*`
    **and** follow the const through the function body: if it is then passed to another of
    *our* functions as `char*`, that parameter becomes `const char*` too (only within your
    directory — else report it). If it bottoms out at an external API taking `char*`
    (SDL/GL/Lua/libpng/libc), cast `(char*)` at that one call with a `// external API takes
    char*` note.
- **`-Wsign-conversion`** — int↔unsigned at a call/assignment/index. **Cast at the
  boundary** with the exact target type from the warning text: `(size_t)`, `(Uint32)`,
  `(GLsizei)`, `(unsigned)`, etc. **Do NOT change the declared type** of any variable that
  appears in arithmetic or comparisons elsewhere (retyping can flip signed arithmetic and
  underflow on this untested code). Retype a variable to the unsigned type **only** if it is
  a pure loop index used solely for indexing/comparison against an unsigned bound and the
  retype adds no new warning. When in doubt, cast.
- **`-Wshorten-64-to-32`** — a 64-bit value (size_t/long/ptrdiff_t/strlen) narrowed to a
  32-bit int. Cast `(int)` at the site (the values here are small and in range). Do not
  widen the destination if that ripples.
- **`-Wpointer-sign`** — almost always an `int` array passed to a GL call wanting
  `GLuint*`/`const GLuint*` (texture/list ids). If the local array is used only for GL ids,
  **retype it to `GLuint`/`unsigned int`** (cleanest). Otherwise cast `(GLuint*)` /
  `(const GLuint*)` at the call.
- **`-Wimplicit-int-conversion`** — int → narrower int. Cast with the destination type.
- **`-Wimplicit-const-int-float-conversion`** — a constant int becoming a float (e.g.
  `INT_MAX`). Make the literal a float, or cast `(float)`.
- **`-Wshadow`** — an inner declaration shadows an outer name. Rename the **inner** one to
  something clear and local (e.g. `i` → `j`, `p` → `q`), updating only its own scope. If the
  inner one is redundant (same value, same purpose), you may remove it. Never rename the
  outer.
- **`-Wunused-but-set-variable` / `-Wunused-variable`** — a local written but never read.
  Remove it **only if its initializer/assignments have no side effects** (a plain value or
  arithmetic). If the RHS is a function call (possible side effect) or it looks like a
  deliberate teaching remnant, keep it and add `[[maybe_unused]]` before the declaration
  instead of deleting.
- **`-Wunused-function`** — a `static` function never called. Do NOT delete (it may be
  teaching scaffolding). Add `[[maybe_unused]]` to its definition.
- **`-Wmissing-format-attribute`** — SKIP these; the coordinator handles them.

## Preservation hazards (opt out, with a reason) — do not "fix" these

- A variable modified between `setjmp` and `longjmp` — do not move or `const` it.
- A loop-carried accumulator (written one iteration, read the next / after the loop) — leave
  its scope alone.
- An address-taken / output-filled local (`&x` passed to be written) — cannot be `const`.
- A `strncpy` whose zero-fill is written to disk/wire — keep it (not applicable to most
  gltron string builds, but check).
- Any deliberate teaching bug or simplification — preserve the behavior and the lesson.

## Report back

List, briefly: each file you changed and the classes you fixed in it; every site you
opted out of and why; and every "needs header change elsewhere: …" you hit. Do not
summarize the rules back.
