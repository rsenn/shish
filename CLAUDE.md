# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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

## Code architecture

The shell follows the classic source → parser → tree → evaluator pipeline.
The whole shell lives under `src/`, with one subdirectory per subsystem and
matching `<subsystem>.h` at `src/`'s root.

- `src/source/` + `src/source.h` — input layer. `struct source` wraps an `fd`
  (file, mmap, string, or stdin/terminal) and feeds the parser. `source_push`
  stacks sources (e.g. `.` / `source` builtin, here-docs).
- `src/parse/` + `src/parse.h` — recursive-descent parser. Token character
  classification is table-driven via `parse_chartable[]` (see
  `parse_chartable.c`) with `parse_is{space,name,ctrl,esc,…}` inline macros in
  `parse.h`. Each grammar production (`parse_if`, `parse_for`, `parse_case`,
  `parse_pipeline`, `parse_simple_command`, `parse_arith_*`, etc.) has its own
  file. Nodes are allocated via `parse_newnode.c`.
- `src/tree/` + `src/tree.h` — the AST. `enum kind` (N_SIMPLECMD, N_PIPELINE,
  N_AND/OR/NOT, N_LIST, N_SUBSHELL, N_BRACEGROUP, N_FOR, N_CASE, N_IF, N_WHILE,
  N_UNTIL, N_FUNCTION, N_ARG*, N_ASSIGN, N_REDIR, plus an `A_*` arithmetic
  sub-tree) drives a tagged `union node`. All node structs are `__packed`.
  `tree_print.c`/`tree_printlist.c` exist for debug/history dumping and can be
  stripped via `NO_TREE_PRINT`.
- `src/eval/` + `src/eval.h` — tree walker. Entry is `eval_tree` /
  `eval_node` / `eval_command`. There's one `eval_<construct>.c` per node
  kind. Control-flow constructs (`break`/`continue`/`return`/`exit`) use a
  `jmp_buf` on each `struct eval` and `eval_jump` to unwind. Loops, function
  bodies and subshells push/pop `struct eval` frames via `eval_push`/`eval_pop`.
- `src/expand/` + `src/expand.h` — word expansion (parameter, command,
  arithmetic, tilde, splitting, glob) applied to `N_ARG*` nodes before
  execution.
- `src/exec/`, `src/fork.c` — process spawn, PATH search, exec.
- `src/redir/`, `src/fd/`, `src/fdstack/`, `src/fdtable/` — redirection. The
  fdtable lazily resolves redirections; per BUGS/TODO this is the source of
  several known issues.
- `src/var/`, `src/vartab/` — variable storage with a stack of `struct vartab`
  per env frame; `var_import` populates from `envp` at startup.
- `src/builtin/` — one C file per builtin (`builtin_<name>.c`).
  `src/builtin/builtin_table.c` is compiled with
  `-DBUILTIN_<NAME>=1` flags derived from `build/.../src/builtin_config.h` so
  the dispatch table only references compiled-in builtins. `builtin_search.c`
  does the name lookup. Two builtin classes: `B_SPECIAL` (POSIX special
  builtins, e.g. `exec`, `exit`, `set`) and default.
- `src/job/` — job control (`SIGCHLD` handler in `sh_main.c`,
  `job_signal`/`wait_nohang`).
- `src/term/`, `src/prompt/`, `src/history/` — interactive terminal layer.
  `term_init` is what flips the shell into interactive mode if `fd_src` is a
  character device.
- `src/sh/` + `src/sh.h` — top-level glue. `struct env` (the per-frame state:
  `cwd`, `umask`, `shopt`, `fdstack`, `varstack`, `arg`, parser, eval, finalizers)
  is the central data structure; the active frame is `sh`. `sh_main.c` does
  argv parsing, env import, source/term setup, then `sh_loop()`.

External dependency `libowfat` lives in-tree at `lib/` and is built as a
static `libowfat.a` by `cmake/libowfat.cmake` (only if no system libowfat is
detected). `libshell.a` is the rest of the shell as a static lib, which both
`shish` and `shformat` link.

## Code style

- C99. Indentation 2 spaces, no tabs, K&R-ish braces (`BreakBeforeBraces: Attach`,
  `AlwaysBreakAfterDefinitionReturnType: TopLevel`). See `.clang-format`.
- Reformat with `cmake --build build/<dir> --target clang-format` or
  `cmake --build build/<dir> --target cmake-format`.
- Headers are guarded with `#ifndef <SUBSYS>_H` and `<subsys>.h` lives at
  `src/`'s root, while implementations live in `src/<subsys>/<subsys>_<verb>.c`.
- Avoid `stdio.h` — use `lib/buffer/`, `lib/fmt/`, `lib/scan/`, `lib/stralloc/`
  helpers from libowfat instead. The shell currently includes `<stdlib.h>` but
  not `<stdio.h>` deliberately.
- `HAVE_CONFIG_H` is always defined; gate platform-conditional code on the
  `HAVE_*` macros emitted into `config.h` (CMake) or `config.h.in` (autotools).

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

## Current focus

Recent and ongoing work, roughly newest-first (see `BUGS`/`TODO.md` and
git log for full detail — this is a pointer into them, not a replacement):

- Rewrote every `tests/*.sh` file to make its checks go through
  `tests/common.sh`'s `assert_*` helpers (see "Tests" above) instead of
  ad-hoc `echo`s nothing was checking against, and disabled the
  `tests/posix`/`tests/yash` CTest registration (still hangs on
  `fnmatch-p.tst`) so plain `ctest` only runs `tests/*.sh` and always
  terminates. Doing this surfaced a run of real, previously-undocumented
  bugs now in `BUGS`: arithmetic expansion rejecting single-character
  variable names (fixed, `fixes/24`), `read -d` leaving the delimiter in
  the captured value (fixed, `fixes/23`), quoted `"$(cmd)"` not
  suppressing field splitting, nested backquotes not working, a quoted
  here-doc delimiter not suppressing expansion, `printf` not supporting
  field widths, and a subshell-bodied function (`f() ( ... )`) not
  actually isolating its variables into a subshell.
- The "command not found"/"not executable" messages added alongside
  the exec-failure-status fixes ignored active redirections on fd 2
  (and fd 0/1): `eval_simple_command.c` prints them before ever calling
  `exec_command()`, which is the only place that resolves a command's
  still-pending (`open()`/`dup2()` deferred) redirections. Fixed by
  resolving `fd_in`/`fd_out`/`fd_err` right before the message. Found
  while writing `tests/fixed.sh`; while testing the fix, also found and
  logged a separate, unrelated bug: quoted command substitution
  (`"$(cmd)"`) doesn't suppress field splitting.
- `break`/`continue` jumping out of `eval`/`.`/`source` left dangling
  `source`/`fd` state behind (a `longjmp` bypassing `eval_pop()`-style
  cleanup), causing a hang or a crash depending on what ran next. Fixed.
- Wired `tests/posix/*.tst` and `tests/yash/*.tst` (yash's own POSIX/self
  conformance suite, 239 files) into `ctest`/`make test` via
  `tests/run-tst.sh`. This now gives a real, concrete POSIX-compliance
  failure list instead of no signal at all — triaging that list is
  unstarted and is the natural next step for POSIX-compliance work.
- Fixed several bugs specifically blocking this repo's own `./configure`
  from running under `shish` (stray errno text on unrelated error
  messages, SIGCHLD/SIGINT staying blocked across fork+exec, a stack-
  use-after-scope in `exec_program()`). `./configure` still doesn't
  complete — duplicated output, a segfault, and a `fdstack_push()`
  consistency-assertion failure are all still open in `BUGS`. This is
  ongoing; the next step is bisecting `configure` itself to narrow down
  which construct triggers it, rather than guessing further.
- Found and fixed a batch of `src/fd*` bugs: fd numbers indexing
  `fdtable[]`/`fd_list[]` with no bounds check (segfault on `exec
  3<&99999`), `<>` truncating a file it should be opening in place,
  `struct fd` leaks from an inconsistent mode-field-overwrite pattern,
  and some dead code.
- Rewrote `src/history/` to lazily `mmap()` + scan the history file
  backward from the end, instead of reading and parsing it fully at
  startup — startup time is now independent of history file size.
- `lib/sig`/`src/job` (job control: `fg`/`bg`, SIGCHLD handling,
  `WAIT_EXITSTATUS`) were surveyed in depth but the fixes haven't been
  applied yet (see `BUGS`) — `fg` in particular has a confirmed stack
  out-of-bounds crash on the plain no-argument invocation, and `bg` is
  an unimplemented stub. Good next target if picking up fresh work here.
