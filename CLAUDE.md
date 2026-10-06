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

## Design metrics

Judge every new module, algorithm and refactor by these four, in this order. A change that makes
one worse has to say so and say why it is worth it; measure, do not guess.

1. **Binary size.** `size` on a `MinSizeRel` build before and after; a feature that grows it needs a reason.
2. **Lines of code and complexity.** Keep both low: fewer lines, fewer branches, fewer special cases.
3. **Memory fragmentation.** Prefer few large allocations with one shared lifetime (arena, one reused
   buffer) over many small ones freed in arbitrary order.
4. **Execution time**, including cache behaviour: contiguous data scanned in order beats pointer chasing.

## Library calls

In `src/`, `lib/` and `text/` call the in-tree functions, never libc's: `str_len()` not `strlen()`,
`byte_copy()` not `memcpy()`, `alloc()` not `malloc()`, and so on. Code samples and comments follow the same rule.

## Types

Design decisions to apply every time, not to re-argue:

- **A boolean struct member is a one-bit field**: `unsigned bgnd : 1;`, never `int`, `char` or
  `uint8_t`. This holds for flags you add and for flags you touch. (`struct shopt`, `struct grep` and
  `struct job` are the model.) A plain local `int` used as a truth value is fine.
- **An integer member takes the type of the value it holds**, not the narrowest type that fits:
  `job->level` is copied from `sh_subshell`, which is an `int`, so it is an `int`; a process count is an
  `int`. Do not shrink members to `uint8_t`/`uint16_t` to save bytes (nothing here is size-critical, and a
  narrow type hides truncation, e.g. a nesting level above 255).
- A "deeper than anything" sentinel is `INT_MAX`, not `0xff`.

## Comments

These rules govern every comment you write or rewrite in this repo, from
here on. Keep comments short and readable, not a running log of debugging
history — and not a packed block of prose either. A comment should be
something the reader's eye takes in as a shape, the way a table or a
diagram is, not something they have to read start to end to parse.

- Struct-member comments: 1-2 lines, right on the member.
- Any other comment explaining behavior: 4 lines max. If the full
  rationale genuinely needs more room, don't write a longer paragraph —
  restructure: a one-line summary, then a short list (one point per
  fact), or the argument table below. Never let a comment become an
  unbroken block of sentences; break it into pieces the eye can scan.
- Never reference `fixes/NN`, `BUGS` entries, issue names, or "confirmed
  via repro X" in a comment — that history belongs in the commit message
  and the `fixes/` patch, not in the source. State the current rule and
  its reason, not how it was discovered or what broke before it existed.
- Prefer showing over telling: when a C expression, a literal value, or
  a short before/after pair makes the point, put that in the comment
  instead of describing it in words. `flags & X_SPLIT` or `"a=="  ->
  "a", ""` says more, faster, than a sentence explaining the same fact.
- If a comment describes a function's parameters, give each one its own
  line: 2-space indent, then type, name, and description as aligned
  columns (pad names so the descriptions all start at the same column):
  ```
  /* one-line summary of what the function does.
   *
   *   const char*  name   what this argument is / controls
   *   size_t       len    what this argument is / controls
   * ----------------------------------------------------------------------- */
  ```
- Write sentences a reader can take in on one pass: subject, verb,
  concrete fact, in that order. No hedging, no throat-clearing, no
  "used to X / now Y" history — state the current behavior and, if it's
  not obvious, the one reason it has to be that way.

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

`text/` holds larger, self-contained text-processing engines (`text/dfa` is
the first: a POSIX BRE/ERE matcher) meant to back builtins like `expr`/
`grep`/`sed`/`awk`. It is neither `lib/` (generic, portable primitives with
no shell dependency) nor `src/` (the shell's own execution engine) — a
subsystem too large and single-purpose to belong in either, built from
`lib/` primitives and consumed by a thin `src/builtin/` wrapper, never a
standalone binary. Everything under `text/` uses `lib/*.h` (`byte.h`,
`alloc.h`, `str.h`, ...) instead of libc `<string.h>`/`<stdlib.h>`, same
rule as `src/` and `lib/` itself.

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
- `DEBUG_OUTPUT`, `DEBUG_COLOR` — verbose debug instrumentation; the trace
  is selected at run time with `SHISH_TRACE` (see `doc/debug-output.md`).
- `BUILD_SHFORMAT=ON` (default), `BUILD_SHPARSE2AST=OFF`.
- `NO_TREE_PRINT=ON` — strip tree-printing helpers from history.
- Builtins are individually toggleable. `cmake/Builtins.cmake` enumerates
  `MINIMAL_BUILTINS`, `DEFAULT_BUILTINS`, `EXTRA_BUILTINS`. `-DBUILTIN_<NAME>=ON/OFF`
  sets one builtin and is permanent (it stays in the cache); `-DENABLE_ALL_BUILTINS=ON`
  turns on every builtin that has no `BUILTIN_<NAME>` of its own, also permanent
  (`=OFF` undoes it). The older `-DENABLE_<NAME>=ON/OFF` spelling is gone: it
  collided with `cmake/LibArchive.cmake`'s own `ENABLE_CAT`/`ENABLE_TAR`/
  `ENABLE_CPIO`/`ENABLE_TEST`/`ENABLE_UNZIP` switches for its bundled tools
  (see `BUGS`). Use `-DBUILTIN_<NAME>` instead.
  The generated `build/.../src/builtin_config.h` is what
  `src/builtin/builtin_table.c` is compiled against.
  Builtins that are normally a coreutils program live in `src/builtin/core/`:
  `basename chmod cp date dirname env expr id link ln ls mkdir mktemp readlink
  realpath rm rmdir sleep split tee timeout touch uname unlink wc`, plus the
  small POSIX utilities `echo false printf pwd test true`. Other utilities
  (`awk digest dirs find hostname popd pushd which xargs`) are in `src/builtin/extra/`, the
  stream filters (`cat cut grep head ...`) in `src/builtin/filter/`.
  Shell-special builtins stay in `src/builtin/`.
  `cmake/Builtins.cmake` (`builtin_source()`) finds a builtin's file in any of
  these; a new coreutils replacement goes in `core/`, and is only compiled
  when enabled.

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

## Debugging with TRACE()

`TRACE()` (`src/trace.h`, `src/trace/`) is the first tool to reach for when a
test fails and the cause is not obvious from the code: it records what the
evaluator decided, as one line per event, and you filter it down to just the
subsystem you suspect. Then use `strace` (what the kernel saw) and `gdb`/
`lldb` (who called) to close the gap. Everything below was run, not guessed.

### Turn it on

```sh
cmake -S . -B build/dbg -DCMAKE_BUILD_TYPE=Debug -DDEBUG_OUTPUT=ON   # or RelWithDebInfo
cmake --build build/dbg -j
```

- `DEBUG_OUTPUT` is only declared when the build type contains `Deb`
  (`Debug`, `RelWithDebInfo`) or `-DBUILD_DEBUG=ON` is passed. With the default
  `MinSizeRel` the flag is **silently ignored** and no trace is compiled in.
- Without `DEBUG_OUTPUT` every `TRACE*` macro expands to nothing: zero cost, and
  the release binary is unchanged. The old `DEBUG_FD/FDSTACK/FDTABLE` flags are
  not needed for `TRACE`; they only feed the legacy `debug.log`.
- `-DBUILTIN_DUMP=ON` adds the `dump` builtin (`-t` fd table, `-s` stack, `-f` list).
- Keep the debug build outside the tree or in `build/` (untracked); rebuild it after
  every source change, or you debug yesterday's code.

### Control it at run time (environment, read once at the first event)

| Variable | Meaning |
|---|---|
| `SHISH_TRACE=fd,fdtable` | modules to trace; `all`; `-name` removes one (`all,-parse`); unset = off |
| `SHISH_TRACE_FILE=path` | default `trace.log` in the cwd, opened `O_APPEND`, **never truncated**; `-` is stderr |

- It must be in the environment when shish starts: `export SHISH_TRACE=...` inside
  a script is not seen. Put it in front of the command: `SHISH_TRACE=fd ./shish -c '...'`.
- `rm -f` the log before each run, or the runs concatenate.
- The file is moved to an fd >= 200 with `FD_CLOEXEC`, so a script that redirects
  fd 1/2 cannot swallow it. `SHISH_TRACE_FILE=-` writes to fd 2: do not use it when the
  script under test redirects stderr.
- One event is one `write(2)` of at most 8192 bytes (longer lines end in `~`), `errno`
  is preserved, and children append to the same file.

### Read the lines

```
[2100940:1] fdtable.dup(from=4, to=1)            TRACE()         a call / decision
[2100940:1] fdtable.gap => r=-3                  TRACE_RET()     its result
[2100940:1] fdtable.exec.fds { 0="/dev/null", 3="pipe:[5184717]" }   TRACE_STRUCT()  a snapshot
```

`[pid:depth]`: the process and the eval nesting depth. `(...)` and `$(...)` run in
the same process (only the depth grows); an external command, a pipeline stage and a
background job each get their own pid, so the pid column tells you which side of a
fork an event happened on.

| Module | What it shows | Reach for it when |
|---|---|---|
| `fd` `fdtable` `fdstack` | `struct fd` push/dup/setfd/pop/close; the virtual-to-effective table; stack levels, pipes | descriptors, redirections, here-docs, `$(...)` output |
| `redir` | redirection evaluation and dups | `>`, `<&`, `exec N>` misbehave |
| `exec` | command lookup, builtin/function/program dispatch, fork and execve | wrong command run, wrong environment, hash cache |
| `eval` | node dispatch, subshell/`$(...)` enter and leave, status | control flow, exit status, `return`/`break`/`exit` |
| `sh` `job` `sig` | forks, exits, job table, signal block/unblock, trap dispatch | hangs, zombies, `wait`, traps, SIGPIPE |
| `var` `expand` `parse` `builtin` | variable ops, expansion, lexer tokens, builtin run/status | wrong values, wrong words, a builtin's argv |

Two events carry the real truth about descriptors:
`fdtable.exec.table(vfd=, shadow=, fd={n=, e=, level=, mode=})` is what the shell
*believes* each virtual fd maps to just before a fork, and `fdtable.exec.fds {N=target}`
is what the child *really* has just before `execve`. A bug is where the two differ.

### Filter before you read

Pick the narrowest set that can contain the cause; widen one module at a time.

1. **By module.** Start at the symptom's own subsystem (table above), not `all`. For a
   descriptor bug: `fd,fdtable`, then add `fdstack,redir`, then `exec`. `all,-parse` is
   the widest that is still readable.
2. **By process.** `grep -a '^\[2100940:' trace.log` is the child you care about;
   the parent's lines are in the same file.
3. **By event.** `grep -a -E 'fdtable\.(dup|gap|wish)|fd\.setfd'`. Cut a long line with
   `cut -c1-200`; drop the table dumps with `grep -v exec.table`.
4. **By window.** Cut between two events that bracket the suspect code:
   `sed -n '/redir.dup(/,/exec.program.execve/p' trace.log`.
5. **By difference.** Trace a passing and a failing variant and diff them after
   normalising what changes between runs:
   ```sh
   norm() { sed -E 's/^\[[0-9]+:/[P:/; s/0x[0-9a-f]+/0xX/g; s/pipe:\[[0-9]+\]/pipe:[N]/g'; }
   diff <(norm <good.log) <(norm <bad.log)
   ```
   The same script traced twice must give an empty diff; if it does not, normalise more.
   The first line that differs is usually the decision that went wrong. Do the same
   against a baseline binary built from `git worktree add -f $SCRATCH/base <commit>`.

### Debug loop (trace -> strace -> debugger)

1. **Minimal repro** as a one-line `-c` script; confirm against `bash`.
2. **Trace** the suspect module; find the first event that is wrong (compare the
   `exec.table` with the `exec.fds` above, or diff good/bad).
3. **strace** the same run to see which syscall each event produced. The trace's own
   `write()`s appear in the strace output, so the two are interleaved for free:
   ```sh
   SHISH_TRACE=fdtable SHISH_TRACE_FILE=/tmp/x/t.log \
     strace -f -o st.txt -s 120 -e trace=write,dup,dup2,dup3,close,fcntl,pipe2,execve ./shish -c '...'
   grep -a -E 'write\(2[0-9][0-9]|dup|close|execve' st.txt     # trace lines are write(200+, ...)
   ```
   Useful tells: a library `close(3)` right after `execve` means fd 3 was
   close-on-exec; a `dup2(a, b)` with no preceding relocation of `b` clobbered a live fd;
   a missing `close()` in a child is a leaked pipe end.
4. **Debugger**, once you know the event: the event name is the breakpoint. Break
   inside `trace_begin` on that event and the backtrace names the caller that emitted it
   (`TRACE(TRACE_FDTABLE, "dup", ...)` is in `fdtable_dup`):
   ```sh
   # gdb
   SHISH_TRACE=fdtable gdb -q -batch \
     -ex 'set follow-fork-mode child' -ex 'set detach-on-fork on' \
     -ex 'break trace_begin if $_streq(event, "dup") && mod == TRACE_FDTABLE' \
     -ex run -ex 'bt 6' -ex 'up' -ex 'info locals' --args ./shish -c 'exec 3>&1; /bin/true'
   # lldb (same idea; mod == 4 is TRACE_FDTABLE)
   SHISH_TRACE=fdtable lldb -b -o 'settings set target.process.follow-fork-mode child' \
     -o 'breakpoint set -n trace_begin -c "(int)strcmp(event, \"dup\") == 0 && mod == 4"' \
     -o run -o 'bt 6' -o 'frame variable' ./shish -- -c 'exec 3>&1; /bin/true'
   ```
   For a line you already know, `break file.c:LINE`, then `print var`; `shell ls -l /proc/<pid>/fd`
   (gdb) lists the process's real descriptors at that moment. After the child `exec`s,
   gdb says "Error in re-setting breakpoint" - harmless, the shell part is done.
   Follow the child with `follow-fork-mode child`; leave it on the parent to debug
   the code that *forks*.
5. **Fix, then re-trace** the repro: the wrong event must be gone and the diff against
   the good variant empty. Add the regression test (see Tests) before you move on.

Complements, not replacements: ASan/UBSan (`-fsanitize=address,undefined`,
`ASAN_OPTIONS=detect_leaks=0`) for memory errors, `valgrind` for the build without a
sanitizer, `ltrace` is rarely useful here (there is almost no libc to trace).

### Add a trace point

```c
TRACE(TRACE_FDTABLE, "dup", trace_int("from", o), trace_int("to", e));   /* decision + inputs */
TRACE_RET(TRACE_FDTABLE, "gap", trace_int("r", r));                      /* result */
TRACE_STRUCT(TRACE_FDTABLE, "state", trace_fd("fd", d));                 /* snapshot */
```

- Name it `module.event`; put the *inputs of a decision* in the call and the *outcome*
  in a `TRACE_RET`. Prefer one event per branch that can go wrong over a dump of everything.
- Value writers: `trace_int`, `trace_hex`, `trace_str` (a NULL string prints `NULL`),
  `trace_argv`, `trace_flags(key, bits, names, n)`, `trace_fd`, `trace_node`,
  `trace_loc`, `trace_kind`; `trace_fdtable(event)` and `trace_fdmap(event)` dump the
  whole table. Add a writer to `src/trace/trace_value.c` for a new type.
- The arguments are only evaluated when the module is selected, but they **vanish**
  without `DEBUG_OUTPUT`: never put a side effect in them, and a variable used only
  inside a `TRACE` may need `(void)` to stay warning-free in a release build.
- From a signal handler never call `TRACE`; use `TRACE_DEFER(mod, ev, key, val)` there
  and `trace_flush()` from normal context.
- A new module is an `enum trace_module` entry in `src/trace.h` plus its name in
  `trace_names[]` in `src/trace/trace_begin.c` (same order).
- Leave useful trace points in. Do not leave `fprintf`/`write(2, ...)`/`abort()` debugging
  in a commit, and check `git diff` for them; keep `trace.log`, `st*.txt` and core files
  out of `git add`.

### Pitfalls

- A stale debug binary: the trace of last build's code looks plausible and is wrong.
- `SHISH_TRACE` unset or the build is not `Deb*`+`DEBUG_OUTPUT`: an empty or missing log
  means the trace is off, not that nothing happened.
- Timing-sensitive races (`sig*`, `wait`) change under `strace -f` and a debugger;
  trace alone perturbs least. Compare the trace of a run with and without `strace`.
- A trace line is written *before* the operation it names finishes if you placed it
  before the call; place the `TRACE_RET` after to see the outcome.

## Design specification for builtin utilities

The POSIX.1-2024 utilities volume
(<https://pubs.opengroup.org/onlinepubs/9799919799/utilities/>, one page per
utility) is the **design specification for every builtin that names a POSIX
utility** (`cat`, `ls`, `sed`, `awk`, `expr`, `test`, ...), and the Shell
Command Language chapter is the one for the special/regular shell builtins.

- Options, operands, stdin/stdout/stderr use, exit status, and
  "unspecified"/"undefined" points are judged against that page.
- Builtins with no POSIX page (`dump`, `digest`, `hostname`, `timeout`,
  `which`, `mktemp`, ...) follow the closest coreutils behaviour and say so in
  their `help_*` text.
- Utilities that are deliberately not implemented (`lex`, `yacc`, `c99`, ...)
  are out of scope; a builtin that omits an *option* of a utility it does
  implement is a discrepancy and belongs in `BUGS` unless listed as a
  documented omission in its `help_*` text.
- Every discrepancy found goes into `BUGS` with a concrete repro command.

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
- `TODO.md` — the roadmap, ordered by the path of least complication: goals, the evidence for why
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

## GitHub Pages site

This project's GitHub Pages site (the `gh-pages` branch) is **generated, not
hand-maintained here**. Use the global `github-pages` skill and the shared site
build tool in the `rsenn/rsenn` repo, at `../rsenn/rsenn` (relative to this repo root;
i.e. `~/Projects/rsenn/rsenn`):

- site definition, landing page, theme, favicon: `../rsenn/rsenn/sites/shish/`
- generator and publisher: `../rsenn/rsenn/tools/site/` (see its `README.md`)
  - build: `qjsm ../rsenn/rsenn/tools/site/build.js shish` (`node` works too)
  - publish: `../rsenn/rsenn/tools/site/sync.sh shish` (commits locally; `--push` only after the user confirms)
- the markdown that becomes the site's pages is **this repo's own** `README.md`,
  `doc/` and `examples/`; a doc page appears on the site only once it is listed in
  `nav` in `../rsenn/rsenn/sites/shish/site.config.js`.

Do not add or extend a `tools/site/`, Pages workflow or `publish.sh` in this repo (any
existing ones are superseded and slated for removal), and do
not edit `gh-pages` by hand.

## Git commits

Omit the `Co-Authored-By: ...` trailer from commit messages. This overrides
any default attribution line Claude Code would otherwise append.
