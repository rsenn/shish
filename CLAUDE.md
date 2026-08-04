# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.

---

## Project

`shish` is a small POSIX-ish shell written in C. It targets the IEEE P1003.2
draft. It deliberately avoids `stdio` and printf-style formatting by linking
against an in-tree copy of Felix von Leitner's `libowfat` (under `lib/`) and
using as few libc facilities as possible (mostly POSIX syscall wrappers). The
codebase is "proof-of-concept" quality per the project's own README and is
**not** a drop-in replacement for `sh`/`bash`.

Two build executables are produced:
- `shish` — the shell (`src/sh/sh_main.c`)
- `shformat` — pretty-printer that reuses the parser (`src/sh/sh_fmt.c`)
- `shparse2ast` — optional AST dumper (off by default; `BUILD_SHPARSE2AST=ON`)

## Build

The repo supports both CMake and autotools, but CMake is the primary path.

### CMake (preferred)

There is no top-level out-of-tree convention; use `cfg-cmake.sh` (sourced via
`cfg.sh`) which dispatches to per-toolchain helpers and writes into
`build/<host-triple>/`.

```sh
. ./cfg.sh                 # source the cfg-* functions into the shell
cfg                        # default native build (writes build/<triple>/)
cmake --build build/x86_64-linux-gnu -j
```

Other helpers in `cfg-cmake.sh`: `cfg-diet`, `cfg-diet32`, `cfg-diet64`,
`cfg-musl`, `cfg-musl32`, `cfg-musl64`, `cfg-mingw32`, `cfg-mingw64`,
`cfg-emscripten`, `cfg-wasm`, `cfg-tcc`, `cfg-aarch64`, `cfg-android`,
`cfg-termux`, `cfg-msys`. Each cross-build writes to its own `build/<host>/`
and may pin a toolchain file.

Common CMake options (pass with `-D…` after the `cfg` function or directly
to `cmake`):
- `CMAKE_BUILD_TYPE={Debug,Release,MinSizeRel,RelWithDebInfo}` — default is
  `MinSizeRel`; any `*Deb*` flips `BUILD_DEBUG=ON` which defines `_DEBUG=1` and
  force-enables the `dump` builtin.
- `LINK_STATIC=ON` — static-link the executables.
- `ENABLE_LTO=ON`, `USE_EFENCE=ON` (debug builds), `WARN_WERROR=ON`.
- `DEBUG_OUTPUT`, `DEBUG_COLOR` — verbose debug instrumentation;
  `DEBUG_PARSE`, `DEBUG_JOB`, `DEBUG_BUILTIN` are per-subsystem (see
  `cmake/Debug.cmake`).
- `BUILD_SHFORMAT=ON` (default), `BUILD_SHPARSE2AST=OFF`.
- `NO_TREE_PRINT=ON` — strip tree-printing helpers from history.
- Builtins are individually toggleable. `cmake/Builtins.cmake` enumerates
  `MINIMAL_BUILTINS`, `DEFAULT_BUILTINS`, `EXTRA_BUILTINS`; pass
  `-DENABLE_ALL_BUILTINS=ON` for everything, or `-DENABLE_<NAME>=ON/OFF`
  per builtin. The generated `build/.../src/builtin_config.h` is what
  `src/builtin/builtin_table.c` is compiled against.

### Autotools (alternative)

```sh
./autogen.sh   # only needed from a VCS checkout (runs aclocal+autoheader+autoconf)
./configure    # detects dietlibc under /opt/diet or /usr/diet automatically
make
make install
```

`configure --enable-builtins="..."` selects which builtins get compiled in
(see `configure.ac` for the master list).

## Tests

Tests are shell scripts under `tests/` (top-level `*.sh`; `common.sh` and
`run-tst.sh` are support files, not tests themselves). CMake registers each
top-level `tests/*.sh` (excluding those two) as a CTest target that runs the
script through the freshly built `shish` binary. This is the only thing
`ctest`/`make test` runs.

```sh
cd build/x86_64-linux-gnu
ctest                          # run every tests/*.sh
ctest -R if.sh -V              # run one test, verbose
./shish ../../tests/if.sh      # invoke a test directly through shish
```

`tests/posix/*.tst` (run through `tests/run-tst.sh`) is wired into `ctest`
by default via `DO_CONFORMANCE_TESTS` (`ON` by default; pass
`-DDO_CONFORMANCE_TESTS=OFF` to skip it for a faster inner loop).
`tests/yash/*.tst` (yash's own POSIX/self conformance suite) is registered
the same way but gated by its own `DO_YASH_TESTS`, **off by default**:
`tests/yash/random-y.tst` hangs and only terminates via its own 120s
per-test `TIMEOUT` (see `BUGS: yash-random-y-tst-hangs`), which otherwise
dominates a default `ctest` run's wall time. Pass `-DDO_YASH_TESTS=ON` to
include it, or run a single file manually without rebuilding:

```sh
sh tests/run-tst.sh ./shish tests/yash some-file.tst
```

### Writing a test

Every `tests/*.sh` file must:

- source `tests/common.sh` (`. "$(dirname "$0")/common.sh"`) and make every
  actual check go through its `assert_equal`/`assert_match`/`assert_nomatch`/
  `assert_greater`/`assert_less` helpers — not ad-hoc `echo`/manual `if`
  blocks with nothing checking the result. Each call takes a `description`
  as its last argument (recommended: say what must be true, not just restate
  the expression) and prints one `<description>: OK`/`<description>: FAIL`
  line as it runs (green/red) — assertions do not stop the script on
  failure, so a single run always shows every check in the file, pass or
  fail.
- end with a call to `summary` (`tests/common.sh`), which prints the final
  tally and is what actually makes the script exit non-zero (so CTest sees
  the failure) if anything failed. A file that doesn't call `summary` at the
  end will report nothing and always "pass" as far as CTest is concerned,
  regardless of what its assertions found.

**Every fix needs a regression test in `tests/fixed.sh`.** Whenever you fix
a bug (whether it started life as a `BUGS` entry or was found and fixed in
the same change), add a case to `tests/fixed.sh` that fails without the fix
and passes with it, plus a patch in `fixes/` (see below) — a fix without a
test protects nothing the next time someone touches that code path. The one
exception is a fix that only compiles/runs on a platform this repo isn't
being developed on (e.g. a `WINDOWS_NATIVE`-only code path) — don't pad
`tests/fixed.sh` with an assertion that's always true just to have a line
item; instead leave a comment there explaining why, and verify the fix by
actually building for that platform (`cfg-mingw64`/`cfg-mingw32` etc., see
`cfg-cmake.sh`) and confirming it compiles and links clean.

`tests/common.sh` defines the `assert_equal`, `assert_match`, `success`,
`failure`, `summary` helpers; each test sources it via
`. "$(dirname "$0")/common.sh"`. A test "fails" by calling `failure` which
prints `FAILURE` and `exit 1`s.

## Tracking bugs and roadmap

This repo tracks known defects and the work plan in two plain files at the
repo root instead of an issue tracker:

- `BUGS` — a flat list of confirmed, currently-open defects, one bullet
  each (dash + short lowercase description, wrapped/indented like the
  existing entries). Only things that are still true belong here. When
  you fix something listed, remove its entry (or narrow it) in the same
  change — don't leave it for later cleanup. When you find a new bug,
  including ones you stumble into while working on something unrelated,
  add it with enough detail to reproduce; a concrete repro command beats
  a vague description every time.
- `TODO.md` — the leverage-sorted roadmap: goals, the evidence for why
  each item matters, what's already been tried and ruled out. Keep it in
  sync with `BUGS` — when a `BUGS` item gets fixed, go update or remove
  the corresponding `TODO.md` mention too, so the two files don't
  quietly drift apart and start contradicting each other.
- `TODO` (no extension) is the old pre-2010 file. Mostly superseded; only
  a couple of genuinely still-open design items remain in it. See
  `TODO.md`'s "old TODO file, investigated" section for the evidence
  trail on why everything else was removed from it.
- `fixes/` — one numbered patch file per fixed bug (`NN-short-name.patch`,
  plain `git diff` output, no commit message), a permanent record of what
  the fix actually was, kept even after the corresponding `BUGS` entry is
  deleted. Add the next-numbered patch as part of the same change that
  removes the entry from `BUGS`, and add its regression test to
  `tests/fixed.sh` (see "Tests" above) in that same change too.

Update both files as part of the change that makes them true, not as a
follow-up — a stale `BUGS`/`TODO.md` is worse than a stale comment, since
the entire point of these files is to be trusted at a glance without
re-deriving the state of the project from scratch.
