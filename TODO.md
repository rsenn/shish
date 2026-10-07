# TODO / Roadmap

Open work only. Done work lives in `git log` and `fixes/*.patch`; confirmed defects with repro steps are in
`BUGS`; how things work is in `doc/`. The sections are in the order to implement them, the path of least
complication: finish what is nearly done, then small independent fixes, then the infrastructure that later work
stands on, then the large plans. The numbers below are section numbers.

**Design metrics** for every change (also in `CLAUDE.md`): binary size, lines of code and complexity, memory
fragmentation, execution time.

| # | Section | Why here |
|---|---|---|
| 1 | Conformance: what is left of Stages 1-2 | the main quest: `tests/posix` is the measurable target |
| 2-3 | Design decisions, `BUGS` map | decide before touching the affected code |
| 4 | Memory safety gate | every later change is checked under ASan+UBSan |
| 5 | Finish the per-builtin switches | almost done; leaves a clean base |
| 6-7 | In-process scope helper, trace leftovers | small, independent, no new modules |
| 8-10 | `sed`/`awk`, `cp`/`mv`, regex engine | feature gaps inside existing builtins |
| 11 | Applet mode | small, needs only the builtin map |
| 12 | Directory walker (`lib/dirlist`, `lib/walk`) | prerequisite of `chown`/`du`/... and removes six hand-rolled walks |
| 13 | `lib/arena.h` in `src/` | leaf call sites first; needs the `stralloc` freeze helper |
| 14 | Optional alias/history/job control | medium, module splits |
| 15 | Binary size | independent, measurable |
| 16-17 | Expansion field list, `src/wordlist/` | **done 2026-10-07** (kept as the record of why and how) |
| 18 | AST arena | parser-wide; `narg.stra` is already gone (section 17) |
| 19-21 | Filter chaining, tab completion, vi mode | interactive and pipeline features |
| 22 | Also open (WASI, editor) | WASI: interactive page plan (xterm.js + WASI shim) |
| 23-25 | More utilities | wait for the walker and the builtin switches |
| 26 | UTF-8 | largest, optional, touches everything |
| 27 | Variable table and `exec_hash` PATH check | independent and measurable; pays off most before 18 (AST arena) |

**Non-goal, decided 2026-09-02:** bash's `var+=value` append-assignment (not in POSIX; `x=a; x+=b` parses
`x+=b` as a command name). It is why libtool's `ltmain.sh` cannot run under shish; libtool-generated scripts are
not a target. Do not add `+=`, not even as an opt-in flag, unless that changes.

---

## 1. Conformance: what is left of Stages 1-2

The measurable target is `tests/posix`; how to run it is in `doc/conformance.md`.

### Where it stands (2026-10-03)

Failing cases per file, everything except the `sig*` family (7 of the 58 files that run
have any; the full `ctest` run has 12 failing tests, 3 of them `tests/*.sh` that need a GNU `date`
or the other known causes in `BUGS`):

```
3 alias-p  62/65    0 param-p 54/54    1 quote-p 34/35    0 case-p 52/52
1 input-p  10/11    1 option-p 74/75   1 simple-p 33/34
```

- `option-p:121` and `simple-p:172` fail on purpose (`BUGS: posix-suite-intentional-deviations`).
- Every runnable `sig*-p` file passes 180/180. The `*3/4/7/8` files, `sigstop`, `sigtstp`,
  `sigttin`, `sigttou` and six others (`bg fg job kill4 testtty wait`) skip themselves: they
  need a controlling terminal.
- **Do not trust a signal-file number from a busy machine.** The same binary scored
  `sigterm1-p` 36/180 in one run and 177/180 in the next; measure on an otherwise idle
  machine and re-measure before concluding anything
  (`BUGS: signal-tests-vary-with-machine-load`).

### How to measure

Moved to `doc/conformance.md` ("Running the suites", "What a family is failing on", "Tests that need a terminal"). Every phase below ends the same way: rerun
the named files, update the table above, remove the closed `BUGS` entry, add `fixes/NN` + a case in
`tests/fixed.sh`.

---

### Phase 3 [Stage 2: builtins/utilities]

1. `alias` (62/65) - the 3 left are in `BUGS: alias-substitution-remaining-cases`.
2. **`set`** - `-b` is accepted and shown in `$-` but has no effect (`BUGS: set-notify-no-effect`);
   `-v` echoes input lines (`BUGS: set-verbose-partial`); `ignoreeof`, `nolog`, `vi` are accepted
   but do nothing (`BUGS: set-o-ignoreeof-nolog-vi-has-no-effect`, `set-histexpand-unimplemented`).

---

### Phase 4/5 [Stage 1: language] - what is left

1. `quote-p` (34/35, only `:431` left) - `BUGS: quote-backslash-escaping-broken` (dash does the same).
2. `input-p` (10/11) - the shell reads ahead inside a command substitution (`BUGS: input-not-read-line-wise`).
3. `simple-p:172` and `option-p:121` fail on purpose (`BUGS: posix-suite-intentional-deviations`).

---

### Phase 6 [Stage 1+2] - what is not being measured at all

1. **The `%REQUIRETTY%` files** (44: the `sigttin`/`sigttou`/`sigtstp`/`sigstop` `*3-p`/`*7-p`/`*8-p`
   combos, `kill4-p`, `bg-p`/`fg-p`/`job-p`, `testtty-p`, `wait-p`) need a real controlling terminal.
   `-DDO_PTY_TESTS=ON` runs them under `tests/pty-run.c` (a single-file POSIX-`pty` wrapper):
   **10/44 pass**, 34 fail. Some are ordinary conformance gaps (`sigcont3-p`'s 3 output
   mismatches), but most `*3-p`/`*7-p`/`*8-p` combos plus `wait-p`/`kill4-p` hang until the 60s
   `pty-run` alarm - a real job-control defect, since their `kill`-driven `*4-p` siblings pass in
   1-2s. One case is traced: `BUGS: wait-interrupted-by-trap-hangs`; the rest need per-case
   bisection (`BUGS: job-control-real-terminal-hangs-vs-kill-driven-ok`). Re-run: `doc/conformance.md`, "Tests that need a terminal".
2. **`tests/yash` (119 files) is off by default**: only `while-y` hangs now (empty loop body, needs
   a POSIX mode; `BUGS: yash-suite-other-hangs`); `arith-y` finishes in ~5s.
3. `grouping-p.tst:34` is flaky (2 of 12 runs): a race between a subshell's background writer and
   the FIFO read after it (`BUGS: grouping-p-tst-flaky`).
4. The harness leaves `tests/posix/tmp.NNNNN/` behind on every hard failure. Clean them up and
   make the harness remove its own.
5. `tests/fixed.sh` gives different results on a default build and with every builtin
   (`BUGS: fixed-sh-assumes-optional-builtins`, `fixed-sh-remaining-failures-after-sigchld-fix`,
   `fixed-sh-fails-under-non-mmap-build`).

---

### Next: remaining `ls` options (`BUGS: ls-missing-options`), then `fc` (`BUGS: fc-missing`)

**Next in line**, in the order they were weighed (see `BUGS` for the repro of each):
`ls-missing-options`, `fc-missing`, then the rest of `BUGS` by repro.

---

## 2. Findings that need a design decision (not fixed, see `BUGS`)

- **`sed N` on the last line** discards the pattern space (POSIX; `tests/builtin-sed.sh` pins it), GNU prints it
  unless `POSIXLY_CORRECT`. `sed N file` on an odd line count therefore differs. Missing GNU sed features found by
  comparing against GNU: `-s`, `-z`, `--expression=`, `--posix`, `-u`, `R`, `F`, `e`.

- **Filter builtins against GNU** (differential run, 2026-10): still different are `cat -n` (spaces; GNU uses a
  tab; `tests/builtin-cat.sh` pins the spaces), `cut --complement`, `grep -r` and `-b`, `sed -s`, and `grep -w`
  does not retry a shorter match at the same start as GNU does. `sort` orders by bytes (C locale), GNU by the
  locale's collation. The long options (`--complement`, `--expression=`) need generic long-option support in
  `filter_init` first (argv must not be mutated).

- Differential run against dash (`${x/b/X}`, `${x//p/r}`, `${x^^}`, `${x,,}`): bash extensions that shish
  accepts but does not apply (`${x/b/X}` prints `abc`). Not POSIX; decide whether to implement or to
  reject them with a diagnostic.
- `command -v echo` prints `/usr/bin/echo` and `command -V echo` says "regular built-in (path)"; dash and
  bash print just `echo` / "is a shell builtin". POSIX allows either for a regular built-in.

- `input-p:89`: `x=$(alias false=:\nfalse)` needs `$(...)` parsed line by line while it runs; shish (like
  dash, which also gives status 1) parses the whole substitution first, so the alias is not seen.

- `while-y.tst` hang: `while echo x; do done` (empty body) is accepted and loops forever; that is the
  case under `posix="true"` in the suite, where a syntax error is expected. `parse_loop()` takes an
  empty `compound_list` on purpose (the non-POSIX cases `while do break; done` and `while ...;do done`
  pass), and shish has no POSIX mode to switch it. Fixing the hang means adding that mode (or
  rejecting empty lists for `sh`/`set -o posix`); the suite also checks yash's exact messages.

- `quote-p:431`: an alias is expanded inside `"$(...)"` in a function body. dash does the same
  (`alias echo=')'; f() { printf '[%s]\n' "$(echo x)"; }; unalias echo; f` prints `[ x)]` in both);
  only yash's suite expects it ignored. Left as is unless the yash behaviour is wanted.

- **`unset -v 1x` status** (`unset-invalid-name-accepted`): shish prints the error and returns 0. bash
  returns 1 and continues; dash exits the non-interactive shell because `unset` is a special builtin.
  A plain `return 1` lands on dash's behavior here (the shell exits), so pick one deliberately.
- **Empty file operand gets no header from `head`/`tail`** (`head-no-header-for-empty-file`): the empty
  source is skipped inside `filter_in_ready()` (`src/filter/filter_in_ready.c`) before `head`/`tail` see it.
  Reporting it means `filter_in_peek()` has to hand out "a file was opened and is empty" (`newfile` set,
  0 bytes), which `grep`, `cat`, `tr`, `compress` and `uncompress` would all have to treat as "nothing yet",
  not as the end of input. Smallest idea: a separate `in->empty` bit that only `head`/`tail` read, set where
  `filter_in_ready()` closes a source that still has `newfile` set.

- **`"$@"""` with no positional parameters gives 0 fields, POSIX (and bash, dash) give 1**
  (`quoted-at-then-empty-quotes-drops-field`; also `"$@"''`): `expand_is_empty_at()`
  (`src/expand/expand_args.c`) treats "only empty literal chunks plus a quoted `$@`" as zero fields. The
  empty chunk after a plain `"$@"` is created by the parser when the quote state switches
  (`src/parse/parse_string.c`: a new `N_ARGSTR` whenever `flag & S_TABLE` differs from `p->quot`), so it
  looks the same as a written `""`. Smallest idea: set a flag bit on an `N_ARGSTR` only where the source
  has an explicit empty quote pair (`""`, `''`), and let `expand_is_empty_at()` ignore only unmarked
  empty chunks. Needs a `tests/posix` run of `quote-p`, `param-p` and `field-p`-style files, since the
  word parser is shared by everything.

- **`break`/`continue` at the top of a file run with `.`** (`source-break-continue-no-op`): bash and dash leave
  the calling script's loop, shish ignores it. `builtin_source()` pushes an `E_ROOT` frame with its own
  `setjmp`, which `eval_jump()` treats as a boundary. `eval` got past this with the `E_EVAL` flag, but a
  jump that skips the source frame would also skip its cleanup (`sh_popargs`, `source_popfd`, `eval_pop`),
  the kind of leak `eval_jump()`'s comment describes. Smallest idea: let `eval_jump()` stop at the source
  frame, longjmp into `builtin_source()` with a "propagate break/continue by N levels" code, and have it
  clean up and re-issue `eval_jump()` from the caller's frame.

- **Here-documents in `tree_cat()`** (`tree-cat-mangles-here-documents`): `redir_addhere()` fills the
  body into the redirection node when it is read, in place of the delimiter word, so the printer has
  neither the delimiter nor the "was it quoted" bit and prints `cat <<"body\n"`. A correct printer needs
  (1) a delimiter chosen at print time (one that does not occur as a line of the body), (2) the quoting
  bit kept on the node (quoted delimiter = no expansion in the body), and (3) the body emitted after the
  next newline of the output, not at the redirection: `cat <<EOF | grep x` has the body after the whole
  line. That is a queue of pending bodies flushed by `tree_catseparator()`, i.e. a change to the output
  order of the whole printer, which `set`'s function dump, `trap -p` and `shformat` all share.

- **Forked function children lose their output inside `$( )`**
  (`timeout-function-output-not-captured-by-command-substitution`): it is not specific to `timeout`.
  `f() { echo hi; }; x=$(f & wait)` also gives `[]` (bash: `[hi]`), while `x=$(f | cat)` works. A command
  substitution collects stdout in a `stralloc`; pipeline members get a real pipe when they fork, but an
  async job and `timeout`'s `exec_command(..., X_NOWAIT)` fork with fd 1 still bound to the `stralloc`.
  The fix belongs where a pipeline stage materializes its pipe (fdstack), reused for X_NOWAIT/`&`
  forks: trace with `SHISH_TRACE=fd,fdtable,exec` on `x=$(f & wait)` and compare `fdtable.exec.table`
  with `fdtable.exec.fds` in the child.

---

## 3. `BUGS` <-> conformance-gap map

**Explains a scoreboard number (fix these as part of Stages 1-2):**

- `signal-tests-vary-with-machine-load` -> section 1 (`sig*-p`).
- `alias-substitution-remaining-cases`, `set-notify-no-effect`, `set-verbose-partial`,
  `set-o-ignoreeof-nolog-vi-has-no-effect`, `set-histexpand-unimplemented` -> Phase 3.
- `quote-backslash-escaping-broken` -> Phase 4.
- `input-not-read-line-wise` -> Phase 5.
- `posix-suite-intentional-deviations` stays as it is.
- `yash-suite-other-hangs`, `grouping-p-tst-flaky`, the three `fixed-sh-*` entries -> Phase 6.

**Real bugs, but not counted in the `tests/posix` scoreboard** (fix opportunistically):
`eval-lineno-imprecise-inside-function`,
`no-tree-print-option-is-a-noop`,
`cfg-cmake-mingw-silently-builds-native`,
`quoted-at-then-empty-quotes-drops-field`.

**Found 2026-10-06/07 while porting expansion and reading `src/var*`** (all in `BUGS` with repros; none counted in the
`tests/posix` scoreboard):
`chmod-argv-memcpy-overlap` (ASan, `chmod -x f`), `nested-break-trips-eval-pop-assert` (Debug builds only),
`unset-leaks-the-var-node` (144 bytes per set+unset, section 27). Fixed on the way: quoted here-document delimiters
with a blank or glob character were never matched (`fixes/378`); `${u}echo hi`, `$e echo hi`, `$((x))` with blanks
and `"${IFS=X}"` mid-command changed behaviour with the wordlist port (section 17, "Step 2 done").

**Memory safety, not conformance** - under "Memory safety" below:
`asan-leak-residue-not-fully-triaged`, `ubsan-buffer-op-proto-function-type-mismatch`.

---

## 4. Memory safety [ongoing] - ASan+UBSan as a recurring gate

A build that has to be run frequently (every fix in Stages 1-2, not just periodically) so a
language/builtin fix doesn't trade a conformance failure for a corruption bug:

```sh
cmake -B build/asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan
(cd build/asan && ASAN_OPTIONS=detect_leaks=0 ctest)    # ASan/UBSan abort = immediate hard failure
```

Open under this build, from `BUGS`:

1. `asan-leak-residue-not-fully-triaged` - 2 allocations per parsed function definition, and the
   process-lifetime function and variable state.
2. `ubsan-buffer-op-proto-function-type-mismatch` - `lib/buffer.h`'s `buffer_op_proto` cast onto
   libc `read`/`write` is UB by the letter of the standard but not fixable without wrapping two
   libc functions everywhere for no observable effect; not planned to change. The two real
   mismatches in unused `lib/buffer/` glob-compiled dead code are left alone per this repo's
   "don't touch unused `lib/` code unasked" standard.
3. **section 6 below**: the repeated per-scope saves are a corruption risk whenever a new
   in-process scope is added.

---

Part A (one name-to-file map, `src/builtin/builtins.map`, drift guard `tests/builtin-map.sh`) is done and
documented in `doc/optional-subsystems.md` ("What was done"); it is no longer listed here.

## 5. Finish the per-builtin switches (Part B, almost done)

**Done:** every file under `src/builtin/` that the map names is wrapped in `#if BUILTIN_A || BUILTIN_B`
(the switches whose line names it as source or in `needs`; generated from the map, `keep` files and
files that already had guards excepted). With every switch off each file compiles to an empty object.
Shared files with per-name guards inside: `dirs`/`popd`/`pushd`, `cp`/`mv`, `break`/`continue`,
`test`/`[` (`[` is `macro=LBRACKET` in the map). `-Wundef` for `src/builtin/` is not enabled: it
gives 166 warnings from `LINK_STATIC`, `WINDOWS_NATIVE` and `GREP_USE_SYSTEM_REGEX` in `lib/` and
none from `BUILTIN_*`; `tests/builtin-map.sh` checks every `BUILTIN_<NAME>` under `src/` against
the map instead. The matrix is not a CTest case; run `sh tests/builtin-matrix.sh [name ...]` after
touching a builtin, and `WITHOUT="test" sh tests/builtin-matrix.sh '['` for a tier m sibling on its own.

**Decided, not planned:** `builtin_digest.c` stays one switch (`BUILTIN_DIGEST`) for its eight names.
It is table-driven (`digest_algos[]`), and the switch also gates the hash-library download in
`cmake/Digest.cmake`; splitting it would touch CMake, `configure.ac` and the tests for no gain.
`compress` and `uncompress` are table-driven over libarchive the same way and stay family switches.

**Left to do:** run the full `tests/builtin-matrix.sh` (one build per builtin name; it configures the minimal
set plus one extra and checks that `shish -c 'type NAME'` works) and the ASan/UBSan gate on the result; then
`-Wundef` for `src/builtin/` once the 166 warnings from `lib/` are cleaned.

---

## 6. One helper for an in-process scope's saves

`(...)` (`eval_subshell()`) and `$(...)` (`expand_command()`) run in this process, and
each repeats the same saves around its body: `fdstack_push`, `fd_state_save`/`restore`
(which also journals fds owned outside the scope), `vartab_push`, `sh_push`,
`exec_functions_save`, `trap_snapshot_save`, plus `sh_sigrestore()` and the job level.
`eval_pipeline()`'s in-process stages repeat a subset. A new in-process scope has to
copy all of them in the right order, and a missed one is a corruption bug.

Do: one `scope_enter()`/`scope_leave()` pair (a struct holding the six saves) used by
all of them, with the `jmpret` handling (`exit` from a real-signal trap re-runs
`sh_exit()`) in one place instead of three. Gate: `tests/fixed.sh`, the posix files and
the ASan build unchanged.

Known limitation: a foreground child that signals the shell by its literal pid inside a
scope is taken to mean "the subshell" (default action); bash sends it to the parent, whose
trap runs afterwards. The two cannot be told apart without separate processes.

---

## 7. Evaluator trace (`SHISH_TRACE`): what is left

The trace layer is complete (`doc/debug-output.md`, `CLAUDE.md` "Debugging with TRACE()").

Open:

- **`SHISH_TRACE` is read from the process environment once**, at the first event; an
  `export SHISH_TRACE=...` inside a running script is not seen. Reading it through `var_get` would
  fix that but touches every event's startup path.
- **Autotools:** works in-tree only (`./autogen.sh && ./configure --enable-debug CPPFLAGS=...`, serial
  `make`). The `src/*/Makefile.in` `MODULES` lists are
  hand-maintained and drift.
- `var.import` is not traced on purpose (one line per environment variable; `var.export` reports the
  count).
- `timeout` forks a function so it can be killed, and then its output is not captured in `$(...)`
  (`BUGS: timeout-function-output-not-captured-by-command-substitution`).

---

## 8. `sed` and `awk`: what is left

Both are `EXTRA_BUILTINS` (off by default) on the shared engines `text/sed/`, `text/awk/` and
`text/dfa/` (`dfa_replace`/`dfa_repl`), `lib/arena` and `lib/hashmap`; tests `tests/builtin-sed.sh`,
`tests/builtin-awk.sh`. Checked against the build on 2026-10-06; everything not listed here works
(`0,/re/`, `first~step`, `q`/`Q` exit codes, `z`, `M` and `I` flags, `y` escapes, `l N`, `T`).

- **`sed`:** `-i`, `-s`, `-z`, `addr,+N` (reports "invalid address"), `e F W R`, and the long options
  `--expression=`, `--posix`, `-u` (need generic long-option support in `filter_init`; argv must not be
  mutated). `N` on the last line discards the pattern space on purpose (POSIX; `tests/builtin-sed.sh` pins it).
- **`awk`:**
  - `END{print NR}` with no main rule does not read the input; `$0`/`NF`/`NR` are lost in `END`.
  - `RS=""` (paragraph mode) and a regex `RS` are not implemented (`RS` is one byte).
  - an empty line with a regex `FS` (`FS="a|b"`) gives `NF=1`, must be 0; the `"\x41"` string escape is missing.
  - `cmd | getline`, `print | cmd` and `system()` are parsed and dispatched through `struct awk_io.run_shell`,
    but `builtin_awk.c` leaves it unset (a clean runtime error: "not supported in this build"); wiring it to the
    evaluator (`$(...)`/`eval` machinery) is a self-contained follow-up.
  - `length`, `substr`, `index`, `match`, `printf %c` count bytes (conformant in the POSIX locale; character
    semantics wait for the UTF-8 plan). `for (k in a)` order is bucket order (unspecified in POSIX).
  - differential driver against gawk: `/tmp/claude-1000/aw.py` is scratch and gone with the machine; rebuild one
    from the cases above if needed.

---

## 9. `cp` and `mv`: what is left

`src/builtin/core/builtin_cp.c` holds `builtin_cpmv()`; the `cp` and `mv` rows (`BUILTIN_CP`/`BUILTIN_MV`,
`EXTRA_BUILTINS`, off by default) point at it, and `cmake/Builtins.cmake` adds `builtin_rm.c` when `mv`
is on (`builtin_rm_tree()` is the exported `rm -r` walk). Tests: `tests/builtin-cp.sh`,
`tests/builtin-mv.sh` (the `EXDEV` cases run when `/dev/shm` is another
file system).

Open:

- `-T`/`-t`; the `SHISH_CPMV_FORCE_COPY` test hook; messages in GNU wording (`target 'x': No such file...`
  instead of `cp: x: not a directory`).
- **Interrupting a copy:** an interactive shell ignores `SIGINT`, so a running `cp` of a large file
  cannot be interrupted; poll for it. Also `ENOSPC` handling and a 4 GiB sparse-file check (skip
  when the file system lacks holes), then the optional `copy_file_range()` fast path if the measured
  copy speed matters; hole preservation.
- **Portability pass:** WASI (Node harness), `cfg-mingw64` (`WINDOWS_NATIVE` rename/attributes; the
  mingw sysroot headers are not installed here, so it is only guarded with `CPMV_NOUNIX`, never
  compiled), musl/diet builds.
- **Size:** measure the pair against two separate entry points (`-DBUILTIN_CP=ON -DBUILTIN_MV=ON`,
  `strip`, `stat -c%s`) and keep the smaller layout.
- Documentation: `doc/builtins.md` entries, short `help_cp`/`help_mv` (section 15.2 counts help bytes),
  README builtin list.
- `ln` used to unlink an existing destination: `cp`/`mv` must not do that either (only with `-f`).

---

## 10. Regex engine `text/dfa`: what is left

Built as `text/dfa/` (header `text/dfa.h`, prefix `dfa_`; not `lib/dfa/`): Pike's NFA simulation
(`dfa_run.c`, one pass, bounded memory) and an explicit-stack backtracker (`dfa_bt.c`) for
patterns with back-references. `expr :`, `grep`, `sed` and `awk` run on it; tests are
`tests/builtin-{expr,grep,sed,awk}.sh`.

Open:

- **Lazy DFA** behind the same entry points (`dfa_test/prefix/search/submatch`) as a pure speed
  mode; not built (so `cbmap`/`hashmap` has no consumer for interning states). Only if a measured
  need appears. It needs a dev-only differential test (random patterns/subjects: DFA vs backtracker
  vs libc `regcomp`, including `(a*)*b`, `(a|aa)*b`, empty loops, 200 000-character subjects) and
  a test that forces the cache flush with a tiny limit.
- **More consumers**, only if wanted: `csplit` (BRE context lines), `ed`, `more` (`/re`), `pax -s`,
  `find -regex`. Not useful: `case`, `${x#pat}`, globbing (`path_fnmatch` is iterative and adequate).
- **Risks that still apply:**
  - backrefs make matching NP-hard: keep them off the NFA path and accept the exponential worst case;
  - the backtracker recurses once per group repetition: cap the depth or make single-atom repeats
    iterative so `\(a\)*` on a long line cannot exhaust the stack;
  - leftmost-longest holds for the overall match; sub-match choice deviates from the strict POSIX
    rule the way glibc does (musl/TRE is the stricter oracle);
  - `\| \+ \?` in BRE (`DFA_GNU`) are implementation-defined; decide with the first real user
    (≈30 lines in the lexer);
  - `sed`'s "empty regex = last regex" is run-time state: keep compiled regexes in the script's
    command array, not on the stack;
  - `w /dev/stdout` works, `r /dev/stdin` is deliberately left out (GNU-only).
- `expr` string comparison (`expr index` is fixed: first position of any character of CHARS).

---

## 11. Busybox-style applet mode: `exec -a cat shish` runs only `cat` (plan)

`ln -s shish cat; ./cat file`, or `(exec -a cat ./build/.../shish file)`, runs the `cat` builtin and exits with its
status; no shell is started. The decision is made from the basename of `argv[0]` alone.

**Which names are applets.** Every builtin whose source is in `src/builtin/*/*.c` (`core/`, `extra/`, `filter/`,
the coreutils-style utilities). The shell-special and shell-state builtins in `src/builtin/*.c` (`cd`, `export`,
`set`, `alias`, `eval`, ...) are never applets: they only mean something inside a shell. The map already carries
this split (the `file` column of `src/builtin/builtins.map` starts with `core/`, `extra/` or `filter/`), so no new
table is needed. The path is stripped (`/usr/bin/cat` -> `cat`); a leading `-` stays with the shell (login name).
A name that is not an applet (`sh`, `shish`, anything else) starts the shell as today.

**Where.** `sh_main.c`, right after the environment is imported into the root vartab (`env` reads it) and before
option parsing: look the basename up in `builtin_table` (new `builtin_applet(name)`, a variant of
`builtin_search()` that accepts only entries whose map file is not top-level), call `fn(argc, argv)` with the
original `argv`, flush `fd_out`/`fd_err` and `exit(status)`. In applet mode `SHELL` is not set, no options are
parsed, no history and no interactive setup run.

**What must stay live.** The shell runtime (fd table, vartab, builtin table): builtins write through `fd_out`, and the
builtins that run another command (`timeout`, `env`, `xargs`, `exec`, `command`) call `exec_command()`, which picks
builtin / function / program by table lookup. `timeout` already forks a builtin without exec
(`exec_command(..., X_NOWAIT)`, `builtin_timeout.c`) and kills the child by pid, so it needs no re-exec.

**What breaks, and the fix.** `exec_program.c` re-runs an `ENOEXEC` script with `execve("/proc/self/exe", sargv, ...)`
and `sargv[0] = argv[0]`; a script named like an applet (`cat`) would turn the re-executed shish into that applet.
`sargv[0]` becomes the fixed name `sh` (`$0` comes from `sargv[1]`, so nothing visible changes).

**Open points.** A map flag for "never an applet" among the `src/builtin/*/` names if one turns out to need the
shell (`dirs`/`pushd`/`popd` keep a stack in the shell: they are in `extra/` but must be excluded); `--help` text
comes from the builtin's `help_*`; `shish cat args` (explicit form) is deliberately not offered, it collides with a
script named `cat`.

**Tests.** `tests/applet.sh`: `exec -a cat "$SHISH_SELF"` and a symlink in a temp dir, stdin through `cat`,
exit status, a builtin with options (`wc -l`), `timeout 1 sleep 5` as an applet, a non-applet name starting a shell,
and the script-named-`cat` case for the `sargv[0]` fix.

---

## 12. One recursive directory walker instead of six hand-rolled ones (Part C, plan)

**Builtins that walk a directory tree today** (found with `grep -l 'opendir\|readdir' src lib text`):

| where | what it walks | order | symlinks | extras it needs from a walker |
|---|---|---|---|---|
| `core/builtin_rm.c` `builtin_rm_tree()` (also called by `mv`) | `rm -r`, cross-device `mv` | children first, then the directory | never followed (`lstat`) | `-f` (ignore ENOENT), `-v`, `-i` prompts *before* descending and *before* removing, "a refused child keeps its parents" |
| `core/builtin_chmod.c` `chmod_path()` | `chmod -R` | directory first, then children | command-line operand followed, nested ones skipped | `-v`, `-c`, `-f`, stat before and after |
| `core/builtin_cp.c` `cpmv_dir()` | `cp -R` | directory created first, then children | `-H` / `-L` / `-P` policy | holds the `DIR*` open while recursing (one fd per level) |
| `core/builtin_ls.c` `ls_dir()` | `ls -R` | listing of a directory, then each subdirectory | `lstat`, never followed | reads all names first, sorts, then closes the directory |
| `extra/builtin_find.c` `find_recursive()` | `find` | directory, then children | `-L` follows, with an ancestor `(dev, ino)` chain against loops | predicates run per entry, `-prune`-like skipping |
| `src/term/term_complete.c` | tab completion | one level only, not recursive | - | wants the same "read a directory into a list" step |

Utilities from `doc/coreutils.md` that need the same walk: `chown -R`, `chgrp -R`, `du` (also `install -d`
in a later pass).

**Plan.** Two layers, both in `lib/` (generic: only `stralloc`, `byte`, `str` and `<dirent.h>`/`<sys/stat.h>`,
no shell dependency; `lib/` is compiled into `libowfat.a`, so a build without a walking builtin does not
link a byte of it):

1. **`lib/dirlist.h` + `lib/dirlist/dirlist_read.c`**: read one directory *whole* into one packed blob, then
   `closedir()`. A record is `type byte, name, NUL`; `type` is `d_type` (`DT_DIR`, `DT_REG`, ...) or
   `DT_UNKNOWN` where the platform has none (mingw, some dietlibc builds). 2 bytes of overhead per entry.
   `.` and `..` are dropped unless `DIRLIST_DOTS` is passed.
   - **No index.** Clients scan: `p += 1 + str_len(p + 1) + 1`; the `str_len` is noise next to the `lstat` or `unlink`
     that follows. `struct dirlist { char* s; size_t len, n; }` carries the byte length and the entry count.
   - **Sorting** (`DIRLIST_SORT`, only `ls` wants it): a temporary `size_t` offset array, sorted with the caller's
     comparator (default `str_diff`), then the blob is permuted into sorted order and the array dropped. Nothing
     persists, so the sorted blob is scanned exactly like an unsorted one.
   - **Ownership.** The blob is built in one scratch `stralloc` that the walk reuses for every directory (one
     malloc per walk), then frozen with `arena_dup` into the caller's `arena`: exact size, contiguous, pointer-stable
     (chunks never move, so `walk_keep()` can hand out names). A level is dropped with `arena_rewind` to the
     `arena_tell` taken before the read; the walk ends with one `arena_free()`. Alternative if the extra copy
     shows up: build directly in the arena with `arena_grow`, falling back to alloc-and-copy at a chunk boundary.
   - **Why not a chain of per-entry nodes:** `next` + `type` + alignment is about 16 bytes per entry instead of 2,
     and a rewind does not lower the peak.
   - **Measured (2026-10-06, tmpfs, 1M files named `file_0000000_abcdefgh`, `/tmp/claude-1000/dl/dl.c`):**
     | variant | listing only | `unlink` each entry | peak RSS |
     | --- | --- | --- | --- |
     | stream (`readdir` + use, nothing held) | 0.19 s | 4.00 s | ~1.4 MB (reported 8 MB) |
     | packed blob, read whole | 0.21 s | 4.07 s | 24 MB |
     | blob + frozen copy (scratch kept) | 0.22 s | 4.21 s | 46 MB |
     `ru_maxrss` has a floor of 8 MB here (an empty directory reports 8112 KB; the exec inherits the launcher's
     peak): a bare process has `VmHWM` 1.3 MB, so the real streaming figure is about 1.4 MB, glibc's 32 KB `DIR`
     buffer included. The blob is 23 bytes per entry (21-char name + type + NUL) and shows at its real size.
     Time differs by 2-5% (the `unlink` dominates); memory differs 3x and 6x. Streaming is the same speed and
     the smallest, and it has no copy. Unlinking while iterating the same `DIR*` was correct here (all 1M
     removed, none skipped; tmpfs only, recheck on ext4 and with a `DT_UNKNOWN` filesystem).
   - **Decision (by the four metrics in `CLAUDE.md`):** the blob stays for `ls` (needs the whole level for sorting
     and column width) and for the walker's *descent*, where a level is read whole so only one descriptor is
     open; but the blob is built directly in the arena (`arena_grow`), not scratch-then-copy, since the
     frozen copy doubles the peak for no gain. For `rm -rf` and `find` over a huge flat directory the 24 MB blob
     is acceptable (about 2.4 MB per 100k entries); a streaming `dirlist_next()` is not worth a second code path.
   - One place to put a `getdents` or `FindFirstFile` backend later; it replaces `ls`'s two `str_dup()` per entry
     and every client's own `readdir()` loop.
2. **`lib/walk.h` + `lib/walk/walk_*.c`**: an `fts(3)` equivalent that does not depend on libc's (dietlibc, musl,
   mingw and wasm have no `fts`; glibc's is not POSIX). It copies `fts`'s *semantics*, not its code:
   an opaque handle (no globals, so a walk may nest inside another, as `find -exec rm -r` does), pull-style
   iteration, and an explicit "skip this one" call.

```c
/* what a visit is about (fts: FTS_D, FTS_DP, FTS_F/SL/..., FTS_DNR/ERR, FTS_DC) */
enum walk_phase { WALK_PRE, WALK_POST, WALK_FILE, WALK_ERR, WALK_CYCLE };
enum walk_flag  { WALK_PHYS = 0, WALK_LOGICAL = 1, WALK_COMFOLLOW = 2, /* lstat / follow all / follow operands */
                  WALK_XDEV = 4, WALK_SORT = 8 };                      /* stay on one device; sort names */

struct walk_ent {
  const char* path;    /* the whole path, in the walker's one growing buffer: valid until the next walk_read() */
  const char* name;    /* last component */
  const char* rel;     /* path below the root operand, no leading '/': "" for the root itself */
  size_t len;          /* str_len(path) */
  int depth, phase;
  int err;             /* errno, when phase == WALK_ERR */
  unsigned type;       /* d_type, or DT_UNKNOWN: filled without a syscall whenever the directory gave it */
  unsigned kept : 1;   /* WALK_POST: something below was kept (walk_keep()) */
};

const struct stat* walk_stat(struct walk*, struct walk_ent*);  /* lstat/stat on demand, as the flags say; cached */
void walk_keep(struct walk*);                                  /* this entry stays: its ancestors report kept */
struct walk;                                                 /* opaque */
struct walk* walk_open(char* const* roots, unsigned flags);  /* NULL-terminated operands, like fts_open() */
struct walk_ent* walk_read(struct walk*);                    /* next visit; NULL when the walk is done */
void walk_skip(struct walk*);                                /* on a WALK_PRE entry: do not descend (fts_set(FTS_SKIP)) */
void walk_close(struct walk*);
```

   - **Order:** a directory is reported `WALK_PRE` before its children and `WALK_POST` after them, so `cp -R`
     (create first) and `rm -r` (remove last) use the phase they need and ignore the other.
   - **One descriptor at a time:** each directory is read whole through `lib/dirlist`, then closed; the walker keeps
     a stack of name lists, not of `DIR*` (the `cp -R` weakness today).
   - **Path buffer:** one growing buffer, as `builtin_rm_tree()` already does, not one per level.
   - **Loops:** an ancestor `(dev, ino)` chain (as `find` already has) reports `WALK_CYCLE` instead of descending.
   - **Errors are visits** (`WALK_ERR` with `err`), so every builtin keeps its own message wording and its own `-f` handling.
   - **Skipping:** `walk_skip()` after a `WALK_PRE` is what `rm -i` (descend refused) and `find -prune` need. The
     `WALK_POST` of a skipped directory is not reported.
   - **No per-directory state:** the destination of `cp -R` is `dst + (path + rootlen)`, derived, not stored.
   - **No callback form** in the library: a builtin that wants one is a three-line `while((e = walk_read(w)))`
     loop. That keeps every `-v`/`-i`/`-f` decision inside the builtin and out of a function pointer with a `void*`.
   - **Not `nftw`:** its callback has no user pointer, it has no portable skip, it cannot sort, and dietlibc/mingw
     lack it. **Not libc's `fts`:** same portability gap, and it is not POSIX.

**Findings from the six walkers (what the walker must do better than they do):**

| client | per level / entry today | with `lib/walk` |
|---|---|---|
| `rm_tree()` | `lstat` + `unlink`/`rmdir` per entry, one `stralloc` path edit per entry, `kept`/`skipped` ints threaded through the recursion | `d_type` says file or directory: `unlink` without `lstat` (2 syscalls to 1 per file); `walk_keep()` and `e->kept` replace the int plumbing |
| `chmod_path()` | `lstat` + `stat` + `chmod`; recursion with a 10-argument signature | octal mode without `-c`/`-v`: no `stat` at all (`chmod` only); symbolic or `-c`: one `walk_stat()` |
| `cpmv_dir()` | `DIR*` held open across the recursion (one fd and one libc 32 KB buffer per level), two `stralloc`s per level | one fd in total; destination is `dst + "/" + e->rel` in a single buffer |
| `find_recursive()` | `lstat` for *every* entry, even for `find . -name x`; ancestor chain always built | `walk_stat()` only when a predicate needs it (`-size`, `-mtime`, `-perm`; `-type` uses `e->type`); cycle chain only with `WALK_LOGICAL`; `maxdepth` stops the descent without reading the directory |
| `ls_dir()` | `lstat` per entry always, two `str_dup()` per entry, a `DIR*` held during the listing | `dirlist` buffer, `lstat` only for `-l -t -S -s -i -F -p` (and for `-R` the type comes from `d_type`) |
| `term_complete` | own `opendir` loop for one level | `dirlist` (one level, no walker) |

   - **No `lstat` unless asked** is the main saving: `find` and `ls` are dominated by the per-entry `lstat`.
   - **Iterative, not recursive:** an explicit stack of frames `{buffer offset, count, next, path length, dev, ino}`
     (about 40 bytes per level) in one growing array, so a 3000-deep chain cannot overflow the C stack and each
     level costs no `malloc`. `dev`/`ino` are only stored for `WALK_LOGICAL`; `WALK_XDEV` stats directories only.
   - **Entries live in the caller's `arena`, the path in one `stralloc`.** Each level `arena_tell()`s before
     `dirlist_read()` and `arena_rewind()`s when it is popped, so a finished level costs no `free()` at all and the
     whole walk ends with one `arena_free()`. Entries are pointer-stable (chunks never move), which is what lets
     `walk_keep()` hand out `e->name` that survives the walk. The path stays a `stralloc`: it must be one
     contiguous, NUL-terminated, growable string for the syscalls, which an arena cannot give. The frames are
     a small array grown with `alloc_re`. The path never contains `.`/`..` (all six clients filter them today).
     Binary size: the arena code is already linked for `text/dfa` and `text/awk`; in a build without those the
     walker adds the ~20 tiny `lib/arena` objects (about 1 KB).
   - **`openat`/`unlinkat`** (when the platform has them) would make every syscall O(1) in the path depth; keep it
     behind one `#ifdef` inside `lib/walk`, exposing `e->dirfd` and `e->name`, so no client changes if it is added.
   - **`ls` stays on `dirlist`:** it must print a directory's whole listing, in its own sort order and after
     `stat`ing the entries, *before* descending, which a pull walker cannot offer; `ls -R` recurses over
     `dirlist` results (`d_type == DT_DIR`) itself.
   - **Expected size:** the six walking blocks are about 85 (`rm`), 25 (`chmod`), 55 (`cp`), 65 (`find`), 40 (`ls`)
     and 12 (completion) lines; each becomes a `walk_open`/`walk_read` loop of 10-25 lines of client logic, so the
     clients shrink by roughly 150-200 lines in total against about 250 lines of library (`dirlist` 60, `walk` 190),
     which is only linked into builds that contain a walking builtin.
   - **Test:** `tests/walk_test.c` over a generated tree: symlink loop (`WALK_LOGICAL` reports `WALK_CYCLE`), an
     unreadable directory (`WALK_ERR`, no `WALK_POST`), a 3000-deep chain, `walk_skip` leaving the directory
     untouched, `walk_keep` marking ancestors, a directory with 20000 entries, `maxdepth`.

**Migration order** (each step keeps that builtin's tests green and is its own patch):
`lib/dirlist` + a unit test (`tests/walk_test.c`, built like `arena_test`) over a generated tree with a
symlink loop, a permission-denied directory and a 3000-deep chain -> `rm` / `builtin_rm_tree()` (also
carries `mv`) -> `chmod -R` -> `find` -> `cp -R` -> `ls -R` (uses `dirlist` only; its per-directory
header/sort/`total` logic stays) -> `term_complete`. New `chown`/`chgrp`/`du` start on the walker.
**Not walkers:** `mkdir -p` and `rmdir -p` follow the *components of one path* and stay on `lib/path`.

**Rejected:** importing `../c-utils/lib/{dir,rdir}`. `rdir_read()` yields a flat pre-order stream of paths;
it has no post-order, no skip, no symlink policy and no loop guard, which is exactly what `rm`, `find`
and `cp` need. Its `dir_open`/`dir_read`/`dir_type` layer (a `FindFirstFile` backend for Windows) is worth
borrowing only if the mingw build ever needs a walker that `<dirent.h>` cannot give it.

---

## 13. Where `lib/arena.h` would fit in `src/`, easiest first (plan)

`lib/arena.h` is already linked (`text/dfa`, `text/awk`), so using it in `src/` adds no new code, only call sites.
The AST arena (section 18) and the expansion field list (sections 16-17) are described below and not repeated here.
Rule of thumb: an arena fits where many small objects share one lifetime and die together, or die in LIFO order.
It does not fit objects freed one by one in any order (history ring, job table, hash entries, variables).

**stralloc and arena** (prerequisite for most items below). `stralloc` touches the allocator in exactly three places:
`stralloc_ready` (`alloc`/`alloc_re`), `stralloc_trunc` (`alloc_re`) and `stralloc_free` (`alloc_free`). Two steps:
1. **Freeze, no change to `stralloc`.** `stralloc_free()` already skips a buffer with `a == 0` ("not owned") and
   `stralloc_ready()` already copies such a buffer out to the heap before growing it. So
   `{ .s = arena_strndup(ar, sa.s, sa.len), .len = sa.len, .a = 0 }` is a valid read-only stralloc today. One helper,
   `stralloc_freeze(arena*, stralloc*)`, builds it and frees the heap buffer. Fix first: `stralloc_trunc` calls
   `alloc_re` on any `s`, so it must copy out when `a == 0`.
2. **Grow inside an arena, only if step 1 is not enough.** A scratch stralloc that lives in an arena needs the arena
   pointer, so a new trailing member `arena* ar` (NULL = heap, so `{0}` and `stralloc_init` stay valid): `ready`
   tries `arena_grow` (works when it is the newest allocation), else `arena_alloc` + copy and leaves a hole;
   `free` becomes a no-op. Costs 8 bytes per stralloc and one branch in three functions. Measure the holes before
   committing to it: scratch strings that are built and frozen at once never leave one.

**Candidates**
1. **`expand_brace.c` / glob** (trivial): **glob part done** (`wordlist_glob` copies the matches into the field
   list's arena and calls `globfree` at once; `expand_glob.c` is gone). `expand_brace.c` still `alloc`s its copies of
   a word; it works on a private tree copy and stays until brace expansion is read-only (section 17, Stage 3).
2. **`eval_simple_command.c`, `eval_pipeline.c`, `exec_program.c`** (easy): **`eval_simple_command` done** (argv is
   `wordlist_argv`, one `arena_tell`/`arena_rewind` per command in `expand_arena`, no `alloca`). Left: the
   `envp`/`sargv` vectors in `exec_program.c` and `eval_pipeline.c` (`alloca` or `alloc` per command, `HAVE_ALLOCA`
   split); `var_count(V_EXPORT)` there is an O(n) walk (section 27).
3. **`term_complete.c`** (easy): `names[]` of `str_dup`s plus six `stralloc_free`s at the end, all one completion
   round. One arena, freed with `arena_reset`; fits the Part C `walk` as its `keep` storage.
4. **`source_alias.c`** (easy): `alias_frame`, `alias_popped` and the alias copies are pushed and popped in LIFO
   order while a line is parsed. `arena_tell` at push, `arena_rewind` at pop. Check `alias_popped` outliving a frame.
5. **`redir_*.c`, `eval_case.c`, `eval_command.c` heredoc** (easy): local `stralloc`s (`sa`, `delim`, `word`,
   `pattern`, `heredoc`) built per command and freed at the end; with step 1 they become one rewind. Low value alone.
6. **`prompt_expand.c`, `sh_loop.c` `cmd`** (easy): one buffer per prompt or per input line; keep as `stralloc`
   (single long-lived buffer is what `stralloc` is for). Not a candidate; listed so nobody tries.
7. **`sh_setargs.c`, positional parameters** (medium): `args->v[i] = str_dup(...)` plus the vector, replaced on
   every `set --`, function call and `shift`. A per-frame arena makes `sh_push`/`sh_pop` a `tell`/`rewind` pair
   and removes the free loop; needs `shift` to drop the first word without freeing it (arena just keeps it).
8. **`exec_search.c` snapshot, `exec_hash.c` path cache** (medium): the node snapshot (`snap->nodes`) lives for
   one lookup (arena fits); the hash entries (`exec_create.c`) are removed one by one by `hash -r`/`PATH` change,
   so they stay on the heap unless `hash -r` is the only way out (then one arena reset).
9. **`parse/` token and word buffers** (medium): each token is a `stralloc` then copied into a node. If the tree
   arena (section 18) exists, the parser allocates the node and the string straight into it and the temporary
   `stralloc`s shrink to one reusable scratch. Do not start before section 18.
10. **`eval_function.c` function bodies** (medium, part of section 18): `nfunc.name = str_dup(...)` plus `tree_copy`
    into a dedicated arena per function; redefinition frees the arena instead of calling `tree_free`.
11. **`var/` and `vartab/`** (hard, probably never): variables are set, unset and exported in any order and
    `var_export` hands pointers to the environment. Only the *value* of a `local` could live in a function-frame
    arena, which needs scope tracking `var` does not have. Leave on the heap.
12. **`history/`** (never): ring of entries freed one by one on overflow. Heap is right.
13. **`job/`, `fd/`, `fdtable/`, `fdstack/`** (never): objects with independent lifetimes and an fd tied to each.
    `fd_filter.c` `FD_BUFSIZE` buffer is one block per filter, nothing to gain.

**Order of work.** stralloc step 1 and the `stralloc_trunc` fix, then 1 + 2 + 3 (each is a leaf and removes more
code than it adds), then 4, 5, 7, 8. Items 9 and 10 belong to section 18. Every step is judged by the same test:
the binary does not grow (`size shish` before and after on `MinSizeRel`) and `alloc`-call counts per command in the
section 18 measurement drop. A step that grows the binary without lowering those counts is not merged.

---

## 14. Make alias, history and job control really optional (plan)

Details, inventories and the graded scenarios are in `doc/optional-subsystems.md`. Short form:

- **alias:** scenario 1 is done (one `struct alias_scan`, a heap list instead of the 8-entry table, `#if BUILTIN_ALIAS`
  at the choke points). **Scenario 3 is open:** module `src/alias/` + `src/alias.h`, one function per file,
  `builtin_alias.c` keeps only `alias`/`unalias` (no `builtin_alias.h`; `history.h` + `src/history/` is the precedent).
- **history:** H1 is DONE (stub header, `needs` entries drop `src/history/` and `term_search.c`). H2 (a read
  interface for the editor, with `fc`) is open; `-H`/`histexpand` stays an ignored flag.
- **job control:** the biggest. `src/job/` is also the shell's process table (every external command, pipeline
  member and `&` goes through it). J1 make interactive job control optional (`JOB_CONTROL` macro,
  `sh_monitor()` constant), J2 split `src/job/` into `src/proc/` (process table) and `src/job/` (jobs/fg/bg,
  terminal, banners), J3 a plain POSIX `proc` implementation when nothing needs the table.
- **Other dead code** (`doc/optional-subsystems.md`, "Other code that is useless when its builtin is off"):
  builtin-only helpers in `src/` (list A there) are still open. `src/filter/` is part of the evaluator and stays.
- **Decisions to take first** (the document's "Open questions"): `set -m` when job control is compiled out;
  whether `wait` may be off; `-H`/`histexpand`.

---

## 15. Make the binary smaller (musl and dietlibc are the targets)

The pitch on the site is "a 185 KB shell". Every number below is
`stat -c%s` on a **stripped** binary, `MinSizeRel` (`-Os`), measured
2026-08-22 at `c44eab01`, gcc 16 / musl-gcc / diet-gcc on x86_64.

```
                         before 5.1   MinSizeRel today   hand-tuned ceiling
glibc, dynamic (default)     189312         142264            136152
musl, static                 237472         195160            191936
dietlibc, static          does not build    152072            149088
```

The middle column is what a plain `-DCMAKE_BUILD_TYPE=MinSizeRel` now
produces; the right one adds LTO, `--icf=all` and `-no-pie`, which are
still opt-in. The dietlibc row is not a typo -- a *static* diet build
undercuts the old *dynamic* glibc one.

### 16.1 Remaining opt-in build flags

Not in the numbers above: LTO (`-DENABLE_LTO=ON`, worth ~8%), `--icf=all`
(needs gold or lld), and `-no-pie` (drops `.rela.dyn`, at the cost of
ASLR for the executable). Stacked on the glibc dynamic build, the tuned
result is 136152 bytes (vs 142264 for plain `MinSizeRel`).

Notes from measuring:

- `--icf=all` needs gold or lld. **gcc `-flto` + `ld.lld` is broken**
  (lld cannot read GCC bitcode: `undefined symbol: main`), and it fails
  *at configure time*, so every `check_include_file` silently reports
  "not found" and the build then dies somewhere unrelated. Use gold
  with gcc; lld only with clang.
- `-DMINSIZE_STRIP=OFF` turns off the post-link `strip` of `.comment`,
  `.note*`, `.eh_frame`, `.eh_frame_hdr`.

### 16.2 Help and usage text: ~13 KB of a 136 KB binary

`.rodata` is 18662 bytes, and the 38 `help_*` strings are 10234 of
them -- 55%. On top: 848 bytes of usage strings and a 1760-byte
`builtin_table` in `.data.rel`. Roughly 10% of a tuned binary is text
that only `help` and usage errors ever print.

Wanted: `-DENABLE_HELP_TEXT=OFF` that nulls the `help`/usage fields of
`struct builtin`. Disabling the `help` *builtin* does not help today --
`builtin_table.c` names every `help_*` symbol, so they all link anyway.

Related, smaller: packing the two `char*` fields into offsets in one
string blob removes 56 relocations from `.data.rel.ro`.

### 16.3 Stop dragging libc subsystems in for one caller each

Measured in the musl static build:

| symbol pulled in | bytes | why | replacement |
|---|---|---|---|
| `pow` (+ libm) | 1916 | `A_EXP` in `expand_arith_binary.c:51` | integer `**` loop -- shell arithmetic is integer, so `pow()` is also a correctness hazard |
| `glob` + `do_glob` + `fnmatch_internal` | ~5200 | `wordlist_glob.c` | the shell already has `path_fnmatch` (1564 bytes); glob = readdir + that |
| `__qsort_r` | 991 | `term_complete.c:60`, sorting completions | insertion sort over a handful of names |

`lib/unix/glob.c` exists but is `#if WINDOWS_NATIVE` only, so every
Unix build takes libc's.

**Plan: an internal POSIX `glob`, and a `USE_LIBC_GLOB` option (2026-09-19).**
Policy for everything shish re-implements that libc also has: use libc's
where it costs nothing (dynamic linking) *unless* the libc function has no
`(ptr, len)` form and we need one — then the internal one is always used.
`fnmatch(3)` is the example: NUL-terminated only, so `path_fnmatch` stays
internal for `case`, `${x%pat}` and (section 10) regex bracket sets.

What each build carries for glob today (all measured 2026-09-19 except
musl, which is the figure above):

| Build | glob-related libc code in the binary |
|---|---|
| glibc, dynamic | 0 B (imports `glob64`/`globfree64`) |
| dietlibc, static | 3469 B: `glob` 1767 + `glob_in_dir` 751 + `globfree` 63 + `fnmatch` 888 |
| musl, static | ≈5.2 KB (table above) |
| glibc, static | 31 KB of code: `glob.o` 11.8 KB + `fnmatch.o` 19.2 KB (`libc.a` members; locale code they pull in not counted) |
| Windows | internal `lib/unix/glob.c`: 1480 B (mingw `-Os`, `WINDOWS_NATIVE` forced) |

1. **Write `lib/glob/` (POSIX flavour)**: `opendir`/`readdir` per path
   component + `path_fnmatch` (with `PATH_FNM_PERIOD`), same `glob()` /
   `globfree()` names and `gl_pathc`/`gl_pathv` as `lib/glob.h`, so
   `wordlist_glob.c` needs no change beyond the include it already selects
   with `HAVE_GLOB`. Estimate ≈150-200 lines, ≈1.2-1.8 KB (Windows
   version as the comparator; not written yet).
2. **CMake `USE_LIBC_GLOB`** = `AUTO` (default): ON for a dynamically
   linked libc that has `glob` (glibc dynamic: 0 B, and libc's glob is
   locale-aware), OFF for `LINK_STATIC`, dietlibc and Windows. When OFF
   the internal one is built and libc's `glob`/`fnmatch` are never
   referenced. Net saving when OFF: ≈2 KB dietlibc, ≈3.5 KB musl, ≈29 KB
   static glibc.
3. **Not identical to libc's glob — decide each, then test:** result
   order (`strcoll` vs bytes; same in the C locale), `[^…]` as negation
   (glibc accepts it, `path_fnmatch` takes only `[!…]`), backslash
   handling, leading-`.` rule, no-match/error return values and the
   `errfunc` callback. In exchange, pathname patterns and `case`/`%`/`#`
   patterns finally use **one** matcher (the open
   `quote-backslash-escaping-broken` entry in `BUGS` is about expansion and quoting, not about
   which matcher runs, so this does not fix it).
4. **Verification:** a dev-only differential script over a fixture tree
   (dotfiles, brackets with classes, escaped metacharacters, symlinks,
   unreadable directories) comparing the internal backend with libc
   `glob64` in the C locale, plus `tests/` cases for what POSIX 2.13
   specifies (`/` never matched by `?`/`*`/`[...]`, leading period matched
   only explicitly, results in collation order, unmatched pattern left
   as is — `wordlist_glob.c` already handles the last). Must pass in both
   `USE_LIBC_GLOB` settings; ASan+UBSan gate as usual.
5. Interaction with section 26: with the internal `glob`, `?` and `[...]` in
   *pathname* patterns become UTF-8-aware for free once `path_fnmatch` is
   (M3), and the `setlocale` question in section 26 (C) disappears for
   static builds.

Other libc duplicates were surveyed and are **not** worth an option
(default glibc-dynamic build, unstripped relink, function sizes from
`nm -S`): `str_*`/`byte_*` are already macros over libc (`lib/str.h:59-61`,
`lib/byte.h:63-68`); thin wrappers `path_getcwd` 100, `path_readlink` 64,
`path_basename` 64, `mmap_read`+`mmap_read_fd` 219, `shell_gethostname`
88 total ≈0.6 KB of ≈107 KB of code; `path_canonicalize` 591 +
`path_realpath` 298 implement the *logical* path (`cd -L`) that
`realpath(3)` cannot; `shell_getopt_r` 350 is reentrant where libc's
`getopt` is not; `fmt_*`/`scan_*` have no libc equivalent without stdio.
One is a correctness question, not a size one: `path_gethome` (281 B)
looks up home directories without libc's `getpwnam`, so a dynamic build
misses NSS-provided users (LDAP etc.) — check before touching it.

### 16.4 `LINK_STATIC` mem-routine switch -- decided: keep the in-tree `byte_*`/`str_*` loops

Re-measured 2026-09: switching the whole family to libc's routines for static builds makes `text` *larger*
(musl +912 bytes, dietlibc +563, glibc identical), because the macros expand at every call site. The loops cost
speed only against an assembly `memcpy` (musl, glibc), and no shell workload shows it. If one turns up, make a
per-function exception (`byte_copy` over `memcpy`), not a per-libc switch.

### 16.5 Builtin set

`-DENABLE_ALL_BUILTINS=ON` costs 26 KB over the default set
(215232 vs 189312 stripped). The `EXTRA_BUILTINS` group (`cat`, `chmod`,
`ln`, `rm`, `mkdir`, `mktemp`, `uname`, ...) is what the container and
agent-sandbox pitch is built on, so it is not obviously droppable -- but
a documented "what does each builtin cost" table would let a distroless
image pick. Largest single builtins, text+data of the object:
`trap` 3987, `test` 3824, `printf` 3778, `expr` 3216, `set` 3120.

### 16.6 Not binary size, but on the same pitch: 262 KB of `.bss`

`sig_stack` 155648, `term_inbuf` 65535, `fdtable_table` 8200, `fd_list`
8192. It costs no file bytes and no RSS until touched, but a shell that
advertises itself for sandboxes should not reserve 155 KB of signal
stack. Worth a look after the above.

### Blockers found while measuring

- `BUGS: no-tree-print-option-is-a-noop` -- an existing size knob that
  does nothing.

### How to measure

```sh
cmake -S . -B /tmp/sz -DCMAKE_BUILD_TYPE=MinSizeRel -DDO_TESTS=OFF \
      -DBUILD_SHFORMAT=OFF <options>
cmake --build /tmp/sz -j8 && strip /tmp/sz/shish && stat -c%s /tmp/sz/shish
size -A /tmp/sz/shish          # per-section, spots .eh_frame-style bloat
nm --size-sort -S -td /tmp/sz/shish | tail -30
```

Always compare stripped sizes, and always re-run `tests/*.sh` with the
result -- `builtin-rmdir.sh` and `fixed.sh` already fail on `main`, so
match against a baseline rather than expecting green.

---

## 16. Word expansion into a field list, not N_ARG nodes (background; done, see section 17)

Independent of the AST arena: the field list owns a scratch arena of its own (see the `wordlist` plan below).

**What expansion is.** `src/expand/` works only on *words*: an `N_ARG` whose `list` holds
`N_ARGSTR`/`N_ARGPARAM`/`N_ARGCMD`/`N_ARGARITH` parts. The result is a chain of `N_ARG`
nodes, one per field, each carrying a growable `stralloc` (`narg.stra`). Consumers only ever
need `(char* s, len)` per field plus one bit ("keep this field even if empty",
`X_QUOTED|X_NOSPLIT|X_SPLIT`): `expand_argv()` for argv, `eval_for.c` for the loop
variable, `expand_vars()` for assignments (always exactly one field). Several entry points
(`expand_str/tostr/tosa/copysa/catsa`, `expand_range`; used by case, redirections, here-doc
delimiters, prompts, `${x:-word}` messages) want a single concatenated string and no fields at
all; `expand_copysa`/`expand_catsa` fake that with a `union node tmpnode` on the stack.

**Measured before Stage 1 (2026-09-20, `valgrind ./shish -c ...`, 1000 loop iterations, control loop
`[ $i -lt 1000 ]; i=$((i+1))` subtracted).** Per execution of one simple command:

| command                  | mallocs | bytes |
| ------------------------ | ------- | ----- |
| `:`                      | 5       | 214   |
| `: abcdefgh` (one word)  | 10      | 436   |
| `: a b c d e f g h`      | 45      | 1927  |
| `: $i`                   | 10      | ~440  |
| `: ${y:-z}`              | 13      | ~535  |

So a literal word cost **5 mallocs, ~215 B** (2 since Stage 1). `set -- $(seq 1 20000); : "$@"` adds
60,029 mallocs / 2.34 MB, about **3 mallocs and 117 B per field** for a ~6-byte string.

**Where the remaining mallocs per word go.**
1. The result is one `N_ARG` node (48 B packed + malloc header) and one `stralloc` buffer sized
   `len + len/8 + 30` (`lib/stralloc/stralloc_ready.c`), so short fields waste most of it.
   `expand_cat()` has seven copies of "`tree_newnode(N_ARG)` + `stralloc_init`".
2. Fields have no lifetime of their own: they live until `tree_free(args)`, after the command.

**Design (each step is its own change; Stage 1, skipping `tree_copy()` when no word needs `{`/`~` rewriting, is done).**

- **Stage 2: an append-only field sink instead of `union node**` cursors.**
  Findings that make this safe (read from `src/expand/`, 2026-09-20):
  - The output is only ever appended at the tail. `nptr` is not a splice point: it is either
    the caller's empty slot for the *first* node, or `&n` of a local holding the current tail
    (`expand_args`/`expand_arg` set `nptr = &n` after every part). The only rewrites are "replace
    the tail field by its glob matches" and IFS splitting (append). Brace expansion splices, but
    on the *input* copy. The returned `union node*` is just the new tail, used as the next cursor.
  - Field boundaries are made two different ways: `X_CATCLOSED` + `expand_cat_sibling()` for IFS
    splitting, and advancing the cursor to an empty slot (`nptr = &n->next`) for `"$@"`. The
    last field of a word is finalized (glob/unescape) in `expand_args()`, all others in
    `expand_cat_finish()`.
  - Input and output share `struct narg` but never mix: input uses `list`, output uses
    `flag`, `stra`, `next` (`id` and `list` are dead on results). Nothing feeds an output node
    back in as input. The `tmpnode` in `expand_copysa/catsa` and the temporary `N_ARG` in
    `expand_arith_expr()` exist only to have a `stralloc` to append to.
  - What consumers need: `expand_argv()` reads `stra.s` and the bits `X_QUOTED|X_NOSPLIT|X_SPLIT`
    (drop an empty field unless one is set); `eval_for` reads `stra`; `expand_vars` reads one
    `stra` per assignment (`var_setsa`); `eval_simple_command` hashes the first field. Nobody
    reads `X_LITERAL/X_GLOB/X_UNESCAPED` after the word is finished.

  Design: the `wordlist` module below.
- **Rule: a field-rewriting step must run at finalization, never as a later pass.** Once the
  arena backs the closed fields, any step that changes an already-closed field's size (glob is
  the existing example; a brace-on-expansion-result extension, which shish doesn't have --
  bash/dash/shish all leave `a='bl{a,e,i}h'; echo $a` unexpanded, confirmed 2026-09-21 -- would
  be another) can only extend that field in place while it is still `top` of the arena, i.e.
  before the next field opens. `wordlist_close()`/`expand_glob()` already run exactly there. A rewrite
  applied as a second pass over an already-built list, after a later field has been appended,
  finds the target is no longer `top`; `arena_grow()` correctly refuses it (see `lib/arena.h`),
  and the only fallback is an O(n) shift of everything after it in the arena -- no cheaper than the
  `realloc()`-based copy the arena was meant to avoid.
- **Side effect on the parse tree.** `narg.stra` is used only by expansion, so `struct narg`
  drops its 24-byte `stralloc`: parse-time `N_ARG` nodes shrink from 48 to 24 bytes.
- **Stage 3 (optional): read-only tilde and brace.** Tilde can be applied when `expand_arg`
  sees a word's first literal chunk (`expand_tilde_lookup()` already returns home + prefix
  length). Brace expansion generates whole alternative words today, so it would become "expand
  the word once per alternative with chunk k replaced". After this `tree_copy()` is only used
  for function and trap bodies.

**Rejected / not now.**
- *Growing `stralloc` inside the arena:* only the newest allocation can grow, and expansion
  interleaves nested expansions (`${x:-$(cmd)}`), so a heap `cur` is the robust choice.
- *Rewriting the escape scheme* (parser doubles glob characters, expansion undoes it with
  `X_LITERAL`/`X_UNESCAPED`/`X_PATTERN`): worthwhile but a separate, risky change.

**Risks.** Expansion is the most patched code in the tree (most `fixes/` touch it), so Stage 2
must not change behaviour. Before starting, capture a characterization test
(`tests/expand-fields.sh`: IFS variants, `"$@"` vs `$@`, `${x:+w}`, empty and quoted-empty
fields, glob with no match, `"a"'b'$c`) with expected values taken from bash and dash, and
compare the `tests/posix` and `tests/yash` pass counts before and after.

**How to measure.** Same commands as the table above (`valgrind ./shish -c '...'`, read
"total heap usage"); run ASan+UBSan too, for dangling fields after an `arena_rewind()`.

---

## 17. `src/wordlist.h` and `src/wordlist/`: the expansion output as its own module (done 2026-10-07)

**Status: all four steps are done; this section is the record. Read "Progress", "Step 2 done" and "Step 4 done" below for
what exists, the rest for why.** The interface sketch and the file list that follow are the original plan; where the
code differs, "Progress" says how.

section 16's Stage 2 (renamed here: `wordlist`, `wordlist_*`) becomes a module of its own, with an arena
behind it. **It does not depend on section 18:** it owns a scratch arena separate from the AST arena. Only the
zero-copy-literal option (pointing a field at a parse-tree string) needs the AST arena.

**Why a module.** Today the output of expansion is a chain of `N_ARG` nodes, so `struct narg` does two jobs:
parse input (`list`) and expansion output (`flag`, `stra`, `next`). Every expansion function takes and returns
`union node**` cursors, `tree_newnode(N_ARG)` appears nine times in `src/expand/`, and `tree_free()` has to run
after each command. After this plan `narg` is parse-only and expansion output is a plain C object.

**Layering.**
```
parse tree (N_ARG, N_ARGSTR, ...)   read-only input, lives as long as the tree
        |  expand_*()               walks the word, decides what to append (src/expand/)
        v
wordlist                            append-only sink: cat / break / close   (src/wordlist/)
        |  wordlist_argv()
        v
char** argv, stralloc, char*        what eval, for, assignments, redirections and case consume
```
`wordlist` knows nothing about `union node`, variables or `$(...)`; `expand` knows nothing about how fields are stored.
`expand_ifs` is passed in (`wl->ifs`), so the module has no dependency on `var`.

**Interface (`src/wordlist.h`).**
```c
typedef struct wordlist {
  arena*     ar;        /* closed fields are frozen here; the caller rewinds it */
  stralloc*  cur;       /* the open field; a pooled buffer, or the caller's stralloc in string mode */
  char**     v;         /* closed fields, v[n] == NULL: this is argv */
  size_t     n, a;      /* fields, capacity of v */
  unsigned   state;     /* X_* bits of the open field (was narg.flag) */
  const char* ifs;      /* splitting characters, NULL = no splitting */
  char*      inl[16];   /* v starts here: up to 15 fields cost no malloc at all */
} wordlist;
void   wordlist_init(wordlist*, arena*, const char* ifs);
void   wordlist_init_str(wordlist*, stralloc* out);   /* string mode: one field, no breaks */
void   wordlist_cat(wordlist*, const char* b, size_t len, int flags); /* the expand_cat() state machine */
void   wordlist_break(wordlist*);   /* end the open field; used by "$@" and IFS splitting */
int    wordlist_close(wordlist*);   /* finish the open field: glob, unescape, keep-or-drop; fields added */
char** wordlist_argv(wordlist*, int* argc);  /* v, NULL-terminated, no copy */
void   wordlist_free(wordlist*);    /* give cur back to the pool, free a spilled v */
```
- **Where bytes live.** The open field is built in `cur`; `wordlist_close` freezes it with `arena_strndup`
  and appends the pointer to `v`. `v` starts in the `inl[]` array (in the caller's stack frame, so `argv` needs no
  copy) and spills to the heap with `alloc_re` only past 15 fields.
- **`cur` is a pooled `stralloc`, not an arena allocation.** `arena_grow` only extends the newest allocation,
  and a failed grow leaves a hole. A tiny pool (one buffer per nesting depth, taken in `wordlist_init`, returned in
  `wordlist_free`) means steady state does no malloc: the buffer keeps its capacity from the last command.
  Starting `cur` on the stack does not work: `stralloc_ready` treats `a == 0` as "not mine, copy out" on the first
  write, which mallocs anyway.
- **Arena choice.** One `arena_heap` arena with 8 KiB chunks, `arena_tell` before and `arena_rewind` after each
  command, so its first chunk is reused for every command. `arena_mmap`/`arena_brk` add nothing for 8 KiB.
- **String mode** replaces `tmpnode`: `expand_str/tostr/tosa/copysa/catsa`, `expand_range`, case patterns,
  redirection targets, here-doc delimiters, prompts, and the temporary `N_ARG` in `expand_arith_expr()` write
  straight into the caller's `stralloc`; `wordlist_break` is a no-op there.
- **State bits.** `X_QUOTED|X_NOSPLIT|X_SPLIT|X_LITERAL|X_GLOB|X_UNESCAPED|X_PATTERN|X_CATCLOSED|X_GLOBRES|X_SUBWORD`
  move from `expand.h` into `wordlist.h` under the same names (no churn at call sites); `expand.h` includes it.
  Only the bits the parser still sets on `narg.flag` stay in the tree.

**Files.** CMake already globs `src/*/*.c`; autotools needs `src/wordlist/Makefile.in` and a `configure.ac` entry.
```
src/wordlist.h
src/wordlist/wordlist_init.c      init, init_str, pool take/return
src/wordlist/wordlist_cat.c       the state machine moved out of expand_cat.c (all fixes/NN logic kept verbatim)
src/wordlist/wordlist_break.c     field boundary
src/wordlist/wordlist_close.c     keep/drop (the X_SPLIT rule), unescape, calls wordlist_glob
src/wordlist/wordlist_glob.c      libc glob() result copied into the arena, globfree() at once (was expand_glob.c)
src/wordlist/wordlist_argv.c      NULL terminator, argc
src/wordlist/wordlist_free.c      pool return, spill free
```
`expand_unescape()` stays in `expand/`: case and redirections use it on plain stralloc.

**`src/expand.h` after the change.** Every function taking `union node** nptr` and returning `union node*` takes a
`wordlist* wl` instead. The returned node was only the new tail, used as the next cursor; a sink has none.

| Today                                                                 | After                                                                |
| --------------------------------------------------------------------- | -------------------------------------------------------------------- |
| `int expand_args(union node* args, union node** nptr, int flags)`      | `int expand_args(union node* args, wordlist* wl, int flags)`  |
| `int expand_vars(union node* vars, union node** nptr)`                 | `int expand_vars(union node* vars, wordlist* wl)`             |
| `union node* expand_arg(union node*, union node** nptr, int flags)`    | `void expand_arg(union node*, wordlist* wl, int flags)`       |
| `union node* expand_param(struct nargparam*, union node** nptr, int)`  | `void expand_param(struct nargparam*, wordlist* wl, int)`     |
| `union node* expand_command(struct nargcmd*, union node** nptr, int)`  | `void expand_command(struct nargcmd*, wordlist* wl, int)`     |
| `union node* expand_arith(struct nargarith*, union node** nptr, int)`  | `void expand_arith(struct nargarith*, wordlist* wl, int)`     |
| `union node* expand_cat(const char*, unsigned, union node** nptr, int)`| removed: `wordlist_cat(wl, b, len, flags)`                           |
| `union node* expand_glob(union node** nptr, int flags)`                | removed: `wordlist_glob`, called from `wordlist_close`               |
| `int expand_argv(union node* args, char** argv)`                       | removed: `wordlist_argv(wl, &argc)`                                  |

Unchanged: `expand_str/copysa/catsa/tosa/tostr` (string mode inside), `expand_unescape`, the brace and tilde
rewrites, `expand_arith_expr` and its siblings, and the `expand_error`/`expand_ifs` globals.
Port check: a few callers use the returned node as "something was appended" (`n = expand_cat(...)`, then
`if(n)`). Look at each one; where the signal matters the function returns `int`, otherwise `void`.

**What changes elsewhere.**
- `eval_simple_command`: `wordlist wl; arena_pos pos = arena_tell(&expand_arena);` ... one `end:` label does
  `wordlist_free` and `arena_rewind`. The `HAVE_ALLOCA` argv block, `tree_free(args)` and `eval_args_top` guard for
  expansion results go away (audit the guard first: it frees `args_head` when a command exits through a nested
  exit; a rewind at the scope that owns the arena replaces it).
- `eval_for`, `expand_vars`, `eval_print_prefix` (xtrace expands the words a second time) take a `wordlist` instead
  of `union node**`. `expand_args(args, nptr, flags)` becomes `expand_args(args, &wl, flags)`.
- `struct narg` drops `stra`: parse-time `N_ARG` goes from 48 to 24 bytes; `tree_free`/`tree_copy`/`debug_node`
  lose their `narg.stra` lines.
- Fields now live exactly as long as today (until the command ends), so nothing that keeps an `argv[i]` pointer
  becomes newly unsafe. Audit once anyway: `hash`, `alias`, `trap`, `export`, `local`, `set --` must copy
  (`sh_setargs` already `str_dup`s).

**Progress (2026-10-06): the module exists** (wired in by step 2, below). `src/wordlist.h` and `src/wordlist/*.c` are built
into `libshell.a` (CMake glob; autotools: `src/wordlist/Makefile.in`, `configure.ac`, `src/Makefile.in`); no caller
links them, so `shish` is byte-identical in size. The `X_*` bits moved from `expand.h` to `wordlist.h` unchanged.
- **Files:** `wordlist_init/_init_str/_free/_pool` (setup, pool of 8 buffers), `_cat` (the `expand_cat` state machine),
  `_break`, `_close`, `_glob` (`expand_glob` on `cur`), `_push`/`_settle` (freeze a field, resolve a pending empty
  one), `_argv`.
- **Differences from the interface sketched above:**
  - `wordlist_glob`, `wordlist_push`, `wordlist_settle`, `wordlist_pool_get/put` are public (internal to the module).
  - `wordlist` carries `has/closed/pend/noglob/oom` one-bit members, `mark` (fields before the current word, so
    `wordlist_close()` can return the count it added) and `own` (the `cur` buffer once all 8 pooled ones are taken).
    `wl->noglob` replaces `sh->opts.noglob`, so the module needs no `sh.h`; it still calls `expand_unescape()`.
  - A field closed by splitting goes into `v` at once; an empty one is entered as `""` and marked `pend`, dropped by
    `wordlist_settle()` unless a sibling follows (`X_SPLIT` is retroactive). No second pass over `v`.
  - `wordlist_close()` re-finishes the last field when the chunk's own splitting had closed it, from the bits it
    collected (`"\\\\ "` unescaped, `"f? "` globbed). `expand_args` does this today through the node flags and
    the output depends on it; it also globs such a field twice, which is not kept.
  - Literal chunks are unescaped while appended (`cat_unescaped`), not through a temporary `stralloc`.
  - On allocation failure the field is lost and `wl->oom` is set; callers check it once, after `wordlist_close`.
- **Verified (harness since deleted):** `tests/wordlist/diff.c` ran random words (text x flags x IFS, with real globbing) through the
  old node chain and the wordlist: 1.5M words, 0 differences, also as three words in a row, in string mode, and a
  100-field spill; clean under ASan+UBSan. 
  `wordlist_break` and `noglob` are not covered by it yet.
- **Measured (`tests/wordlist/bench.c`, since deleted, steady state, one expansion = one command, MinSizeRel flags -O2):**

  | per command                         | node chain      | wordlist   |
  | ----------------------------------- | --------------- | ---------- |
  | `: a b c d e f g h`: malloc calls   | 16              | 0          |
  | `: a b c d e f g h`: bytes malloced | 632 (768 in 16 blocks) | 0   |
  | `: a b c d e f g h`: instructions   | 6118            | 3619 (-41%) |
  | `: a b c d e f g h`: data refs      | 2472            | 1417 (-43%) |
  | `$x` of 8 fields: instructions      | 6524            | 3689 (-43%) |
  | `$x` of 8 fields: ns (wall)         | 750             | 360        |

  The wordlist's working set is one pooled buffer, one arena chunk (reused, rewound) and the argv in the caller's
  frame, so the micro benchmark is all L1 hits for both; D1 misses are 0.00 per command either way. The real-shell
  runs are under "Step 2 done" below (time, mallocs, fragmentation); a direct cache measurement is still missing
  (`perf` is not installed on the development machine; `valgrind --tool=cachegrind` only shows L1 hits for this
  working set).
- The two items that were open here (`NO_GLOB` glob linking, `wordlist_argv()` replacing `expand_argv()`) are done.

**Step 2 done (2026-10-07): the shell expands through `wordlist`.** Words, assignments, `for` lists, the `set -x`
prefix and `$((x))` all use it; `eval_simple_command` has no node chain, no `alloca` argv and no guard any more.
- **Wiring:** `expand_arena` (8 KiB heap chunks) is defined in `expand_args.c`; `eval_simple_command`,
  `eval_for` and `eval_print_prefix` take `arena_tell()` and `arena_rewind()` it. `expand_arg/param/command/arith`
  take a `wordlist*` and return `void`; a failed part ends its word through `expand_error` (saved and restored
  per word, so an earlier word's error stays). `expand_vars` writes one `name=value` field per assignment
  (never globbed). `expand_copysa/catsa/tosa` and `expand_arith_expr` use string mode. The pool in
  `wordlist_pool.c` is now an unbounded stack of buffers (`wordlist_pool_mark/_release`).
- **`eval_args_unwind.c` is gone** (with `struct eval_args`, `eval_args_top`, the guard): `struct eval` keeps
  `apos` (`arena_tell`) and `pool` from `eval_push`, and `eval_jump`, `eval_return`, `eval_exit` rewind both before
  the longjmp. A 200000-iteration loop of `break`/`return` through commands with expanded words stays at 6 MB.
- **Behaviour changes (all fixes):** `${u}echo hi` runs `echo` (the old chain dropped the whole word when its first
  part was empty); `$e echo hi` looks up `echo`, not `""`; `$((x))` with `x="1 2"` is an error instead of using `2`;
  `"${IFS=X}"` re-reads IFS for the rest of the command (the yash `fsplit-y` case keeps passing).
- **Checks:** `expand-fields.sh` 131/131; `fixed.sh` the same 5 known failures (plus 3 new cases); ctest fails
  the same 6 tests with the same counts; `tests/yash` (all but `random-y`) identical to the baseline binary file by
  file; ASan+UBSan run of `fixed.sh` and every `tests/*.sh` clean in `src/expand`, `src/eval`, `src/wordlist`.
  Found on the way, not caused by this: `BUGS: chmod-argv-memcpy-overlap`, `nested-break-trips-eval-pop-assert`.
- **Measured** (real shell, `: a b c d e f g h $v1 "$v2 x" $((n+1))` in a `while` loop, MinSizeRel, all builtins):

  | | before | after |
  | --- | --- | --- |
  | wall time, 300000 iterations | 1.26 s | 0.69 s |
  | mallocs per iteration (valgrind) | 43.0 | 3.0 |
  | bytes malloced per iteration | 1688 | 104 |
  | maxrss | 6048 KiB | 6044 KiB |
  | `size` text / data | 376720 / 13704 | 374381 / 13800 |

  Fragmentation (`tests/wordlist/frag.sh` + `mallinfo.c`: 400 live variables of mixed size, 200000 iterations of
  expansions, assignments, `set --`, `for`, `case`, `$(...)` and a variable rewritten every 20th iteration; glibc
  `mallinfo2()` at exit; deterministic, identical on repeat runs):

  | at exit | before (`f20235ab`) | after |
  | --- | --- | --- |
  | heap size | 270336 | 270336 |
  | in use | 220000 | 225392 |
  | free in heap | 50336 (18.6%) | 44944 (16.6%) |
  | free chunks (not fastbin) | 48 | 7 |
  | fastbin chunks | 284 | 251 |
  | wall time | 2.84 s | 2.42 s |
  | maxrss | 6428 KiB | 6444 KiB |

  So the heap does not get smaller (glibc's initial heap dominates at this size), but it is far less shredded: 7
  free blocks instead of 48. The cache claim is still not measured directly (`perf` is not installed here).
- **Step 4 done (2026-10-07):** `expand_cat.c`, `expand_glob.c`, `X_CATCLOSED` and `narg.stra` are gone (parse-time
  `N_ARG` 48 to 24 bytes, one malloc and one free less per parsed word; `tree_copy`, `tree_free`, `debug_node` lost
  their `stra` lines); the `NO_GLOB` `lib/glob.c` lines moved to `src/wordlist/Makefile.in`. `tests/wordlist/diff.c`
  and `bench.c` went with the old chain (the 1.5M-word comparison above is the evidence they produced).
  `size` text 374337 (baseline 376720); ctest, yash suite and ASan+UBSan as after step 2. `redir_eval`'s wrapper
  `N_ARG` stays: it is a tree node for the tilde helpers, not expansion output. `param-assign-default-not-split` is
  untouched.

**Order of work.** Each step builds, passes `tests/posix` + `tests/yash` counts unchanged, and is its own commit.
0. DONE: `tests/expand-fields.sh` (131 assertions, every value agreed on by bash and dash, passes under all three
   shells). It found one deviation, `BUGS: param-assign-default-not-split`, which is not in the file.
1. DONE (string mode went in with step 2): `expand_str` (and `expand_tostr` on top of it) use string mode; `tmpnode` is still
   in `expand_copysa/catsa`, `expand_tosa` and `expand_arith_expr`. **These cannot move first:** they call
   `expand_arg()`, which hands `union node**` cursors to `expand_param/command/arith`, so they port together with
   step 2.
2. DONE: port `expand_cat`'s state machine and `expand_glob` into `wordlist_cat`/`_close`; `expand_args` and `expand_argv`
   fill a wordlist; `eval_simple_command` switches over. This is the one risky step.
3. DONE (with step 2): `eval_for`, `expand_vars`, `eval_print_prefix`; delete the nine `tree_newnode(N_ARG)` sites in `expand/`
   (`expand_brace.c:218` builds input copies, not output, and stays until brace becomes read-only, Stage 3).
4. Arena scoping and the pool (until now `ar` may be a plain heap-backed arena reset per command); remove
   `narg.stra`.

**Expected result** (estimates from the section 16 table, to be re-measured): `: a b c d e f g h` from 45 mallocs and
~1.9 KB in ~8 separate blocks to 0 mallocs in steady state and ~20 bytes of contiguous arena (`len + 1` per field);
`: abcdefgh` from 5 to 0. Less fragmentation because per-word buffers (`len + len/8 + 30`) and nodes, freed
in tree order, become one bump region released by a single rewind. Code size: the node plumbing and
`tree_free` branches go, the module adds about 200 lines; accept only if `size shish` does not grow.

**Rejected.** Growing `cur` inside the arena (nested `$(...)` allocations interleave; see section 16 "Rejected");
one arena per word (rewind granularity is the command); making `wordlist` know about variables or the tree.

**Risks.** Same as section 16: expansion carries most of `fixes/`, so step 2 must not change behaviour and the state
machine moves verbatim. A dangling `argv` after `arena_rewind` is the new failure class: run ASan+UBSan and keep one
test that expands inside a function called from a `$(...)` inside an assignment.

---

## 18. Arena allocator for the AST (`lib/arena` exists; `text/` uses it, `src/` does not yet)

`src/tree.h`'s AST is a graph of individually `malloc()`'d nodes
(`tree_newnode()`) plus separately `malloc()`'d string buffers hanging off
several of them — one `malloc`/`free` pair per node, even though a tree's
real lifetime is always "parse it all at once, evaluate, throw the whole
thing away" (`sh_loop.c`). `lib/arena.h` is a generic bump allocator (pluggable
source, `arena_tell()`/`arena_rewind()` for nested lifetimes, `arena_grow()` for the newest
allocation only); only `text/dfa` and `text/awk` use it so far, nothing in `src/` does.

Design decisions already worked out (full reasoning in git history —
2026-07-23/24 commits):

- **One arena with marks, or a stack of arenas.** Every independent
  parse-evaluate-free scope (`sh_loop.c`, `builtin_eval.c`,
  `builtin_source.c`, `builtin_expr.c`, `prompt_parse.c`,
  `builtin_trap.c`'s inline parse) nests strictly via ordinary call-stack
  recursion — shish is single-threaded, so arenas never need to overlap
  without nesting. Push one per scope; `arena_reset()`/`arena_free()` it
  wherever `tree_free()` is called today; `arena_tell()`/`arena_rewind()`
  gives the same nesting inside a single arena.
- **`tree_free()` mostly disappears, not just changes signature.** Most of
  its current call sites just free a subtree still inside the current
  statement — those calls simply go away, since the dead nodes just wait
  for the enclosing arena to reset. Only the handful of true scope
  boundaries above get an `arena_reset()`/`arena_free()` call instead.
- **Two things can't live in the transient arena:** function bodies and
  trap bodies, since both must outlive the statement that defines them.
  Trap bodies already parse through their own independent `parse_init()`
  call, so they can just get their own dedicated, never-reset arena.
  Function bodies parse inline as part of the defining statement and are
  deep-copied into long-lived storage at adoption time by `tree_copy()`
  (`src/tree/tree_copy.c`, mirrors `tree_free()`'s per-kind switch). Once
  the arena lands, `tree_copy()` must switch from allocating loose nodes
  to bump-allocating into the function's own dedicated arena.
- **`stralloc` doesn't fit an arena** — it grows via `realloc()`, which
  can't work once other data has been bump-allocated after it. Two ways
  in: the parser keeps building in its one reusable heap `p->sa` and
  freezes the result with `arena_strndup()` (one copy, no waste); or, only
  when the string is the newest allocation, `arena_grow()` extends it in
  place and `arena_trim()` freezes it. `arena_grow()` never copies (that
  would leave a hole); it returns NULL and the parser falls back to `p->sa`.
  This covers the tree's own write-once-at-parse-time strings:
  `nargstr` (as its `strview view` overlay of `stra`), `nargparam.name`,
  `nfor.varn` and `nfunc.name` (populated once during parsing).
  (`narg.stra` no longer exists; section 17.) Packing a node and its string tightly
  adjacent in the arena is safe with no alignment padding, since
  `src/tree.h`'s node structs are already `__packed`.
- **Expansion results are a separate problem, now solved separately.** Words expand into a `wordlist` in
  `expand_arena`, not into the parse arena or into nodes (section 17, done).
- **Possible future: precompiled/cached AST on disk.** Serialize arena
  blocks with node pointers rewritten to offsets; on load, run one linear
  fixup pass turning offsets back into real pointers (structured like
  `tree_free()`'s own `switch(node->id)`) — after that, every existing
  tree-walking function works completely unmodified. A more invasive
  "offsets natively everywhere, zero-copy `mmap()`" design is possible but
  touches every tree-walking call site for a benefit unlikely to matter
  next to lexing/parsing cost.


### Leverage and size of the migration (measured 2026-10-07, not implemented)

Measured by parsing `tests/fixed.sh` (6415 lines) with `shformat` (tree kept) and `shish -n` (parse, then `tree_free`),
callgrind and valgrind, MinSizeRel build.

| | measured |
| --- | --- |
| mallocs for one parse | 52816: 28130 nodes, 20664 string buffers, about 4000 other |
| tree held in memory | 2.18 MB of heap in 40632 blocks (`mallinfo2`) |
| instructions, parse + `tree_free` | 100.7M (about 15.7k per line); allocation and free are roughly a quarter: `tree_free` about 16.6M, `alloc` about 8.7M |
| `parse_word`'s `stralloc_catc` | 472000 calls, 10.5M instructions, about 10% of the parse |

By design metric:
1. **Binary size:** small. `tree_free` (811 bytes of text, 213 lines) goes, about 36 call sites with it (10 are parser
   error paths); `tree_copy` stays (function bodies are copied into their own arena). The tree objects are 2.7 KB in
   all. Expect about 1 to 1.5 KB; unmeasured.
2. **Lines and complexity:** about 213 lines out of `tree_free.c` and 36 free call sites, about 100 lines of arena
   plumbing back: a few hundred lines fewer overall.
3. **Fragmentation and memory:** the largest gain. A string buffer is `len + len/8 + 30` bytes today (36 for a
   5-character name) plus a malloc header; `arena_strndup` makes it 6. Nodes lose their 8 to 16 byte headers. Estimate
   for that file: about 1.1 MB in the arena against 2.18 MB on the heap, roughly half (an estimate, not built).
4. **Execution time:** about 20% of the parse and free instructions, which is about 3.9k instructions per line or
   1 ms per 1000 lines of script. Scripts spend their time in `fork`/`exec`, so end to end it barely shows; it
   matters for very large scripts and for `eval` in a tight loop.

Size of the refactor: `tree_newnode` is called from 23 files (28 sites) and takes an arena; `tree_free` at 36 sites,
`tree_copy` at 10 (function definitions, traps, and the per-execution tilde/brace copies); `nargstr.stra` is used at 62
sites, 25 of them in `expand_tilde.c`, which rewrites text in place on private copies, so `nargstr` moving to the
`strview` it already overlays means those rewriters work on copies. The risk is lifetimes, not volume: function and
trap bodies outlive their statement; `exec_function_retire` already defers freeing bodies that are still running and
has to survive as a deferred `arena_free`; nested `eval`/`source` need strictly nested arenas; every longjmp landing
needs the rewind that section 17 added for `expand_arena` (`eval_jump`, `eval_return`, `eval_exit`).

**Order, cheapest and most certain first:**
1. **Per-execution tree copies into `expand_arena`.** `expand_args`, `expand_vars`, `expand_param`, `eval_case` and
   `redir_eval` copy a subtree and free it on every execution of a command containing `~` or `{a,b}`. An arena-aware
   `tree_copy` (copy into `expand_arena`, no free; the rewind per command already exists) removes about 8 sites of
   malloc and free per execution, a cost that grows with the loop count, unlike parse cost.
2. **`parse_unquoted`/`parse_word`: copy runs, not characters** (separate from the arena): the 472000 `stralloc_catc`
   calls are about 10% of the parse, probably as many instructions as the whole arena saves.
3. **The parse-tree arena itself,** for the memory halving, only if the lines and size savings above still look
   worthwhile after 1 and 2; one arena per parse scope as described above.

### `stralloc` slack in the tree and elsewhere, and `strview` (measured 2026-10-07, not implemented)

`stralloc_ready` allocates `len + len/8 + 30` bytes, a minimum of 31 for a one-byte string, and glibc rounds a 31 to 40
byte request up to a 48 byte chunk (`n + 8` rounded to 16, at least 32). The rule suits strings that grow; most of these
never do. `valgrind --tool=dhat` attribution by caller:

| where | blocks | finding |
| --- | --- | --- |
| parse tree word text (`parse_string` into `nargstr.stra`) | 12588, 11102 live | about 10 characters each, 48 byte chunks instead of 32 |
| parse tree parameter names (`parse_param`) | 1915 | each exactly 31 bytes |
| all live `stralloc` after parsing `fixed.sh` | 13058 blocks, 527 KB | at least 392 KB (74%) is slack |
| `var_setsa` (plain `x=...` assignment) | 19576 in 3000 loop iterations (40% of all) | frees the variable's buffer and mallocs a new one every time; only `var_setv` reuses capacity |
| `$((n+1))` operands (`expand_arith_expr`, string-mode `value`) | about 18400 | one 31+ byte malloc and free per operand |
| `case` subject and pattern (`eval_case`, `expand_catsa`) | 3006 | one buffer each, 143 bytes in the benchmark |
| `var_init` name buffers | 527 | 31+ bytes each |
| `sh_push` | 150 | copies `cwd` for every `$(...)` or subshell |

Not measured: the line editor and history (`src/term`, `src/history`, about 20 files use `stralloc`) and builtins such
as `read` and `command`.

**Fixes, cheapest first:**
1. `var_setsa` reuses the buffer when the new value fits (as `var_setv` does): about 6 malloc/free pairs less per
   loop iteration in the 400-variable benchmark.
2. `$((...))` operands and `eval_case` scratch take a pooled buffer (`wordlist_pool_get()`, which keeps its
   capacity): 0 mallocs.
3. `parse_string`: when the node's string is still empty (the common case), allocate exactly `len + 1` and copy;
   later chunks keep the growing path. Same number of mallocs, about 16 bytes less per block on about 11000 blocks:
   roughly 180 KB, 8% of the 2.18 MB tree. A shrink pass at the end of the word does **not** work: glibc cannot split
   a 48 byte chunk to 32 in place (the 16 byte remainder is below the minimum chunk), so the first allocation has to
   be the right size.
4. A smaller global minimum (8 instead of 30) would help everything, but `parse_string` and others append a character
   at a time, so it means more regrowth; fix the callers instead.

**Could the tree use only `strview`?** `nargstr` already has the union (`stralloc stra` / `strview view`, `tree.h`),
and `tree_cat` is the one reader of the view side.
- **Type change alone: about nothing.** A view is 16 bytes against 24, so `nargstr` goes from 56 to 48 bytes, but glibc
  rounds both to a 64 byte chunk (live: 12986 blocks of 56 bytes, `nargstr` and `nargparam`). It only pays together
  with `struct location` (16 bytes, token position) shrinking to 8: 40 bytes is a 48 byte chunk, 16 bytes less on
  about 11000 nodes (about 180 KB).
- **With the arena it is the real prize.** A packed 48 byte `nargstr` with its text directly behind it in the same bump
  allocation is about 58 bytes per word part; today it is a 64 byte node plus a 48 byte string, about 112. Roughly
  half for those nodes, about 700 KB of the 2.18 MB.
- **Obstacles.** `parse_string` merges several chunks into one node and a here-document body appends line by line, so
  the parser keeps building in `p->sa` and freezes the string when the node is complete (exact-size appending would
  be quadratic on long bodies). The in-place text rewriters (`expand_tilde.c` about 25 of the 62 `nargstr.stra` uses,
  `expand_brace.c`, `tree_copy.c`) work on private copies and would write new views into `expand_arena`, which is
  rewound per command anyway.
- **Order:** fix 3 above now; fold the `strview` conversion into the arena migration (step 3 of the list above), not
  before it.

---

## 19. Pull-based filter chaining: what is left

`eval_pipeline()` chains a pipeline's whole non-last prefix straight into the true last stage through
in-process buffers (`FD_FILTER`, `struct filter_ops`, `pipeline_filter_prepare_chain()`) when every
non-last stage is a filter-capable builtin with literal argv; a runtime decline (`grep -c`/`-q`) rolls
the chain back and the pipeline forks as before. No `fork()`/`pipe()` runs for `cat file | grep -E '(a|b)' |
sed '...'`. Both `sed` and `grep` are optional builtins, so nothing outside
`src/builtin/{extra,filter}/builtin_*.c` may name them, and every build must still compile with either
or both off.

Open:

- **`!HAVE_FORK`/WASI builds get correctness, not just speed, from this.**
  `eval_pipeline_sequential()`'s per-stage full-materialization fallback (`eval_pipeline.c`) hangs on an
  infinite producer (`yes | sed ... | head` never finishes stage one). An `FD_FILTER` chain needs no
  `fork()`/`pipe()` for its own stretch, so decide whether `eval_pipeline_sequential()` should try
  chaining first and fall back to full materialization only when the chain is not entirely steppable
  builtins. It does not attempt chaining yet.

### Which builtins would benefit from being a filter (2026-09-27)

Filter-capable today: `cat`, `grep`, `sed`, `sort`, `head`, `tail`, `uniq`, `cut`, `nl`, `tr`, `paste` and the compress/uncompress family. A chain needs *every* non-last stage to be one (the last
stage already runs in-process and reads the chained buffer), so a single non-capable builtin anywhere
in the prefix (`echo x | awk ... | sed ...`) sends the whole pipeline back to `fork()`+`pipe()`.
What matters is therefore position: **producers** (first stage, no stdin) and **middle stages**.

| builtin | role | verdict | how |
|---|---|---|---|
| `echo`, `printf` | producer | **highest value**: `echo "$x" \| grep ...`, `printf '%s\n' ... \| sort` are the most common first stages of all | eager (see below), ~10 lines each |
| `awk` | middle | **high**: `grep ... \| awk ... \| sed ...` is a standard idiom; `text/awk` already reads through `fd_in` | streaming, like `sed`'s filter (`awk_state` reads records) |
| `tee` | middle | **high**: `cmd \| tee log \| next`; side effect (write files) plus pass-through | streaming: copy in `read()`, write the files as bytes pass |
| `find`, `ls` | producer | medium: `find . -name '*.c' \| grep ...`, `ls \| wc -l` | eager |
| `set`, `alias`, `export -p`, `readonly -p`, `trap`, `type`, `command -v`, `jobs`, `umask`, `pwd` | producer | medium: `set \| grep ^X`, `alias \| sed`, `jobs \| wc`; read-only views of shell state | eager |
| `basename`, `dirname`, `realpath`, `readlink`, `which`, `uname`, `hostname`, `expr` | producer | low: normally used inside `$(...)`, not a pipeline | eager, free once the adapter exists |
| `wc`, `digest` | sink | low as a filter: a sink emits one line at EOF, and as the *last* stage it already chains; only `x \| wc -l \| y` benefits | eager |
| `xargs`, `timeout`, `env`/`nice`/`nohup` (planned) | runs another command | **no**: the output belongs to the executed command (real fds, forked child), nothing to hand back in-process | none |
| `cd`, `read`, `export`, `set` (assigning forms), `.`, `eval`, `exit`, `mktemp`, `sleep`, `kill` | state / no stdout | **no**: side effects on the shell must happen in the shell, a chained stage runs lazily and possibly never | none |
| `date`, `id` (builtins, not filters yet); planned: `du pathchk` | producer | as `find` | eager |

**One adapter covers every "eager" row.** A generic `filter_eager` ops table in
`src/builtin/builtin_filter.c` runs the builtin's normal entry point on the first `read()` with `fd_out`
redirected into a `stralloc` (the same `FD_SUBST` mechanism `$(...)` uses) and, for a consumer, `fd_in`
set to the upstream buffer; then `read()` hands the bytes out and `status()` returns the exit code.
A builtin opts in with one table entry (`&filter_eager`), no per-builtin code. Trade-offs: the whole
output is held in memory, and an eager stage does not stop early (`yes | head` would never end, so
`yes` stays out); real streaming stays with `cat grep sed head ...` (and `awk`, `tee` once they are filters). It must decline (`open()`
returning NULL, nothing printed) for anything that changes shell state, which is the "no" rows above.

---

## 20. Tab-completion: context-aware, and extensible through `complete`

**Not started beyond the first slice; this section is the plan.** (The
file is `src/term/term_complete.c`, 363 lines; there is no
`term_completion.c`.) Today TAB completes *file names*, and, since the
first-word change, also reserved words that begin a construct, builtins
and function names at the first word of a command (`tests/term-complete.sh`).
This goal turns that into a completion engine with three layers:
**what kind of word is under the cursor** (context), **where candidates
come from** (sources), and **who may add sources** (`complete`/`compgen`/
`compopt`, bash-compatible subset).

### What exists and what is missing (probed 2026-09-20 with a pty driver)

| Input, TAB at `|` | Today | Wanted |
|---|---|---|
| `ls "my fi|` (file `my file`) | nothing | `ls "my file"` — quote-aware word split |
| `ls my\ fi|` | nothing | `ls my\ file ` — escape-aware |
| `cd |` | lists files and dirs | directories only |
| `echo $HO|` | nothing | `$HOME` (variable names) |
| `pytho|` (first word) | nothing | executables from `$PATH` |
| `don|`, `fi|`, `esac|`, `els|`, `the|` | nothing | the closer/continuer, **when valid** |
| `for x i|` / `case x i|` | nothing | `in` |
| `if true; th|` | nothing | `then` |
| `echo x > a|` | files, dirs unmarked | files; dirs get `/` in the list |
| `kill -|`, `trap '' |`, `fg %|`, `unset V|` | file names | signals / signals / jobs / variables |
| `\` in a filename, `~user/`, `$VAR/x` | partly (`~/` only) | all three |
| a candidate list of 3000 entries | prints all | asks first ("Display all N?") |

Facts about the code that shape the design (all checked 2026-09-20):

- TAB is one `case '\t'` in `term_read()` (`term_read.c:184`). The editor
  holds only the **current line** (`term_cmdline`); earlier lines of a
  multi-line command were already handed to the parser, so "am I inside an
  `if`?" is not visible to the editor today.
- `struct parser` is a local object created by `parse_init()`, not a
  singleton, so a second parse over a string is possible; but
  `parse_error()` prints through `sh_msg()`, so a dry run needs a silent mode.
- Running shell code from inside the editor has a precedent: `prompt_show()`
  expands PS1 (which may contain command substitution) while `term_read()`
  is active.
- Data already there: `builtin_table[]`, `functions` (linked
  `union node`, name in `nfunc.name`), `parse_aliases`, the variable table
  (`var_search`/`var_hsearch`), `job_list`, `exec_hashtbl` (only the *hashed*
  commands, not all of `$PATH`), `sig_name()`, `expand_tilde_lookup()`.
- shish has **no arrays** (`a=(1 2)` is a syntax error), so bash's
  `COMP_WORDS`/`COMPREPLY` arrays need an equivalent (see Phase 5).
- `term_complete.c` is compiled out on `WINDOWS_NATIVE`; no `opendir`
  fallback there.
- Candidates are `str_dup`'d one by one and de-duplicated by a linear scan;
  fine for hundreds, quadratic for a `$PATH` with 3000 commands.

### Design

```
 TAB ─▶ compl_words(line, pos)   split with quotes/escapes, find the word under
                                  the cursor, keep its raw and unquoted form
      ─▶ compl_context(words, pending)   classify → CTX_COMMAND / CTX_ARG(cmd, n) /
                                  CTX_REDIR / CTX_VAR / CTX_TILDE / CTX_JOB /
                                  CTX_IN / CTX_KEYWORD_POS(state)
      ─▶ compl_spec(ctx)         pick a spec: user-registered (`complete`) first,
                                  else a built-in default for that command/context
      ─▶ compl_generate(spec, cur, &set)   run the sources, filter by prefix
      ─▶ compl_apply(set)        common prefix, suffix, list, escape on insert
```

- **One candidate set** (`struct compl`): a `stralloc` arena plus an offset
  array (no per-name allocation), sorted once, de-duplicated after the
  sort; flags per set — `nospace`, `filenames` (quote/escape on insert,
  `/` for directories), `dirs_only`. The name/source of each candidate is not
  kept; kind is only needed for the trailing character, which a set-wide
  flag plus one `stat()` for the unique-match case already gives.
- **`compl_generate()` is a pure function** of `(line, pos)` that fills a set
  and prints nothing: TAB calls it, `compgen` calls it, and tests call it
  without a pty (Phase 0 adds `compgen --line 'LINE' [POS]`, a non-bash
  debugging switch that prints one candidate per line).
- **Defaults are specs in the same format as user specs**: the built-in
  knowledge ("`cd` takes directories", "`kill` takes signals or jobs") is a
  compiled-in table of `struct compspec` rows, so Phase 5's registry is
  "the same table, with more rows added at run time" and a user can override
  any default with `complete -r cd; complete -A file cd`.

`struct compspec` (Phase 3 defines it, Phase 5 makes it user-visible):

```c
struct compspec {
  const char* name;      /* command this applies to; NULL = default (-D), "" = empty line (-E) */
  unsigned actions;      /* CA_ALIAS | CA_BUILTIN | CA_COMMAND | CA_DIR | CA_FILE | CA_FUNCTION |
                            CA_EXPORT | CA_JOB | CA_KEYWORD | CA_SIGNAL | CA_USER | CA_VARIABLE |
                            CA_SETOPT | CA_HELPTOPIC | CA_RUNNING | CA_STOPPED ... */
  const char* words;     /* -W: word list, expanded at completion time, split with $IFS */
  const char* func;      /* -F: function name */
  const char* cmd;       /* -C: command (needs fork: not on WASI) */
  const char* filter;    /* -X: glob; matching candidates are removed */
  const char* prefix;    /* -P */
  const char* suffix;    /* -S */
  unsigned opts;         /* -o: default dirnames filenames noquote nosort nospace plusdirs */
};
```

### Phase 0 — refactor and test harness (no behaviour change)

1. Split `term_complete.c` along the diagram above, one function per file as
   in the rest of `src/`: `compl_words.c`, `compl_context.c`,
   `compl_src_file.c`, `compl_src_names.c` (keywords/builtins/functions/
   aliases/variables/jobs/signals), `compl_set.c` (arena, sort, dedupe,
   common prefix), `compl_apply.c` (insert/list/redraw). `term_complete()`
   stays the public entry and becomes ~15 lines. Keep `WINDOWS_NATIVE`
   compiled out as today.
2. Introduce `struct compl` and make the existing file/keyword/builtin/
   function sources fill it. The de-dup becomes sort-then-uniq.
3. Add `compgen --line 'LINE' [POS]` (dev switch) and move the existing
   `tests/term-complete.sh` assertions to it; keep **a few** pty cases for
   the editor glue only (insertion, redraw, list), from a shared
   `tests/pty-drive.py` instead of the copy embedded in the test today.
   Gate: `tests/term-complete.sh` result unchanged before/after.

### Phase 1 — every reserved word at command position (the requested change)

Today the first-word list is `case do elif else for function if then until while`.
Add the rest so any keyword can be completed:

| Word | Offered at | Note |
|---|---|---|
| `done` `esac` `fi` `}` `)` | command position | closers; Phase 4 narrows to "only when one is open" |
| `else` `elif` `then` `do` | command position | continuers; Phase 4 narrows |
| `in` | **not** a command position: the word after `for NAME` and after `case WORD` | needs the previous words (Phase 2's splitter, or a 20-line special case first) |
| `{` `!` | command position | `{` needs a following blank (`{ ` is inserted) |
| `[[` , `time` | not in shish; do not offer |  |

Rule for this phase: offer the full set whenever the word under the cursor is
in command position and non-empty (bare TAB still lists files only). No
state tracking yet — Phase 4 adds it, and keeps this list as the fallback
when the tracked state offers nothing, so `fi<TAB>` can never be worse than
here. Tests: one assertion per word, plus `for x i<TAB>` → `in`.

### Phase 2 — quote- and escape-aware words; correct insertion

`compl_words()` replaces the "back to the previous blank" scan:

- Splits the line prefix into words honouring `'…'`, `"…"`, `\x`, `$(…)`/
  `` `…` `` nesting depth, and operators `; & | ( ) < > &&  ||`; each word
  keeps `raw` (as typed) and `cooked` (quotes removed) and the **open quote**
  at the cursor (`0`, `'`, `"`).
- Candidates are matched against `cooked`; insertion re-quotes according to
  the state at the cursor: unquoted → backslash-escape blanks/metacharacters
  (`my file` → `my\ file`); inside `"…"` → escape only `" $ \ ``; inside
  `'…'` → close the quote and reopen for a `'` (`'\''`). A unique match
  finishes the quote (`"my file" `), except with `nospace`.
- Directory part: expand `~`, `~user` (`expand_tilde_lookup`) and `$VAR` for
  the directory lookup only; the line keeps what the user typed (as `~/`
  does today).
- Trailing character rules in one place: dir (`stat`, follows symlinks) → `/`,
  file → space, `nospace` → nothing, inside an open quote → the closing quote.
- Listing: directories are shown with a trailing `/`; sorted with `strcmp`
  (C locale) — locale collation is out of scope; width by
  `mb_cols` once section 26 exists.
- First TAB inserts the common prefix; only if nothing was inserted (or on a
  second TAB) print the list, like bash; more than `COMPLETION_QUERY_ITEMS`
  (default 100) candidates asks "Display all N possibilities? (y or n)".
- Fixes `BUGS: term-complete-quoted-word` (the `ls "my fi|` and
  `ls my\ fi|` rows above both silently do nothing today); remove the entry
  and add a `tests/fixed.sh` case with the fix.

### Phase 3 — context and sources (what to complete where)

**Contexts** (from the word list; the classification is a `switch` on the
previous word / operator, not a parser):

| Context | Recognised by | Sources |
|---|---|---|
| command | first word; after `; & \| && \|\| ( ` ` { ! then do else elif if while until` | keywords (Phase 1), builtins, functions, aliases, `$PATH` executables (below) |
| command prefix | previous word is `command` `exec` `nohup` `env` `xargs` `time` `nice` `sudo` `builtin` `exec` | as *command*, one word later |
| redirection target | previous word matches `[0-9]*[<>]`, `>>`, `<&`, `>&`, `<>`, `>\|` | files; **no** `$PATH` |
| variable name | after `$`, `${`, `${#`, `unset`/`export`/`readonly`/`read`/`local`/`typeset` operands, and `NAME=` left of `=` | variables (`var_hsearch`); `$`/`${` insert braces only if a `}` context |
| assignment value | after `NAME=` | files (`x=~/pr|`) |
| tilde | `~foo|` | users via `expand_tilde_lookup`-style lookup (`getpwent` where libc has it; skip on dietlibc/WASI) |
| job | `%|` , operands of `fg bg wait kill disown` | `job_list`: `%1`, `%+`, `%-`, `%name` |
| signal | `kill -|`, `kill -s |`, `trap … |` | `sig_name()` table: `INT`, `SIGINT`, numbers |
| directory | operands of `cd pushd popd rmdir` (and `mkdir -p` parents) | directories only; `cd` also `$CDPATH` |
| option | word starting `-` after a builtin that has a `struct builtin_cmd.args` string | flag letters parsed out of `args` (`"[-lp] [[arg] signal_spec ...]"`); `--help` |
| `set -o`/`+o` | | option names from the `set` table (`builtin_set.c`) |
| `type`/`command -v`/`which`/`hash`/`help`/`unalias`/`alias` | | commands / commands / commands / commands / builtins / aliases |
| `in` (after `for NAME`, `case WORD`) | | the keyword `in` |
| default | everything else | files |

**`$PATH` executables** (new; nothing completes commands from `$PATH` today):
a cache keyed by the `PATH` string and each directory's `st_mtime`;
each entry is name only (no `stat` per file — `opendir` + `d_type`/`access(X_OK)`
lazily on first use of a prefix, then reuse); cache is dropped when `PATH`
changes or `hash -r` runs. Budget: first TAB on a `$PATH` of ~3000 commands
must stay under ~30 ms; measure (`How to measure` below). Entries already in
`exec_hashtbl` are a subset and add nothing.

**Defaults are `struct compspec` rows** (table above, one row per builtin
family), so the engine has a single dispatch path; per-row cost ≈ 40 bytes.

### Phase 4 — which reserved words are valid here (state across lines)

Goal: `fi<TAB>` only when an `if` is open; `then<TAB>` after `if list;`; `do<TAB>`
after `while list;` or `for … in …;`; `done<TAB>` inside a loop body;
`esac<TAB>` inside `case … in`; `else`/`elif` in a then-part; `}` inside `{`.

Three ways to know the open constructs, compared:

| | Idea | Multi-line | Same line | Cost/risk |
|---|---|---|---|---|
| A | mini-lexer over `pending + line[:cursor]` with a construct stack | needs `pending` (below) | yes | ~150 lines, independent of the parser; can drift from the real grammar (aliases that expand to keywords, `case` patterns, here-docs) |
| B | instrument the live parser: a global stack pushed/popped in `parse_if/loop/case/for/grouping/function` | yes, exact | **no** — tokens of the current line are not parsed yet | touches 6 parser files; still needs A for the current line |
| C | real parser dry run over `pending + line[:cursor]` with `P_COMPLETE` + silent errors; `parse_expect()` already receives the set of acceptable tokens (`toks`) | yes | yes, exact | needs silent `parse_error`, an EOF-at-cursor mode that returns the expected set instead of failing, alias-expansion side effects contained; ~120 lines in `src/parse/`, highest risk |

**Recommendation: A first**, C only if A's drift shows up in practice.

- `pending`: the editor keeps the text of the previous lines of the
  *current unfinished command* (`term_pending`, appended when a line is
  submitted while `prompt_number == 2`, cleared when the parser finishes a
  command or on ^C). No parser change; a few lines in `term_read.c`.
- The mini-lexer reuses `compl_words()` and adds a **construct stack**
  driven by the reserved word in each command position:
  `if`→IF_COND, `then`→IF_THEN, `elif`→IF_COND, `else`→IF_ELSE, `fi` pops;
  `while/until`→LOOP_COND, `for`→FOR_HEAD, `do`→LOOP_BODY, `done` pops;
  `case`→CASE_HEAD, `in`→CASE_BODY, `esac` pops; `{`→BRACE, `}` pops;
  `(`→SUBSHELL, `)` pops; `name()`/`function`→FUNC_HEAD. Words inside `case`
  patterns, quoted strings, `$( )` and here-doc bodies (`<<EOF` … `EOF`) are
  skipped, not interpreted.
- Valid-next table (the stack top decides):

| Top of stack | Command-position words offered |
|---|---|
| none / BODY states | starters: `if while until for case function { ! ( ` and commands |
| IF_COND, LOOP_COND | commands, plus `then` / `do` **only after a list terminator** (`;` `&` or newline) |
| IF_THEN | commands, `elif`, `else`, `fi` |
| IF_ELSE | commands, `fi` |
| LOOP_BODY | commands, `done` |
| FOR_HEAD | after `for NAME`: `in` or `do` (`;`/newline first) |
| CASE_HEAD | `in` |
| CASE_BODY | `esac`, patterns (no completion) |
| BRACE | commands, `}` |

- `alias unless=if`: if the first word of a command is an alias whose
  first word is a keyword, treat it as that keyword (one level, via
  `parse_aliases`).
- Fallback: when the valid set contains nothing matching the typed prefix,
  offer **all** reserved words anyway (Phase 1 behaviour), so a lexer error can
  never remove a completion that worked before.

### Phase 5 — extensibility: `complete`, `compgen`, `compopt`

A bash-compatible **subset** (so existing completion scripts with
`complete -F` mostly work), off-by-default builtins in `EXTRA_BUILTINS` until
the size is known (`ENABLE_COMPLETE`); the editor side (Phases 0-4) does not
depend on them.

**Builtins**

| Command | Behaviour |
|---|---|
| `complete [-abcdefgjksuv] [-o opt]... [-A action]... [-F func] [-C cmd] [-W words] [-X filter] [-P prefix] [-S suffix] name...` | register a spec for each `name` |
| `complete -D` / `-E` / `-I` | default spec / empty-line spec / initial word (first word being completed) |
| `complete -p [name...]` | print registered specs in re-usable form (`complete -F _foo foo`) |
| `complete -r [name...]` | remove specs (no name: all) |
| `compgen [options] [word]` | print the candidates the given spec would give for `word`, one per line; exit 0 if any, 1 if none; same options as `complete` (no `name`) |
| `compopt [-o opt] [+o opt] [name...]` | change options of a registered spec, or of the completion in progress when called from `-F` |

Actions (`-A`): `alias builtin command directory export file function
hostname job keyword running setopt signal stopped user variable helptopic`
(`arrayvar binding disabled enabled group service shopt` are not planned:
no arrays/readline/`enable`/`shopt` in shish; `-A` with an unsupported action
is an error, exit 2). Letter forms as bash (`-a -b -c -d -e -f -g -j -k -s
-u -v`). `-o`: `default dirnames filenames noquote nosort nospace plusdirs`
(`bashdefault` accepted and ignored).

**Dispatch** (first match wins): spec registered for the exact command word,
then for its basename, then `-E` (empty word on an empty line), then `-D`,
then the built-in default row (Phase 3). The "command word" is the first word
of the simple command after prefix words (`sudo`, `command`, …, from Phase 3).
`-o default` falls back to file completion when the spec yields nothing.

**Function protocol** (`-F func`), adapted to a shell without arrays:

- Called as `func cmd cur prev` (`$1` command name, `$2` word being completed,
  `$3` the word before it), as bash.
- Variables set for the call: `COMP_LINE` (whole line), `COMP_POINT` (cursor
  byte offset), `COMP_CWORD` (index of the word being completed, from 0),
  `COMP_WORDS` (**newline-separated** words — deviation: no arrays),
  `COMP_TYPE` (`9` normal, `63` list on ambiguity), `COMP_KEY` (`9`).
  They are unset again afterwards.
- Result: the function sets `COMPREPLY` to a **newline-separated** string,
  or `compgen … ` writes to stdout and the function runs
  `COMPREPLY=$(compgen -W "$words" -- "$2")` — the idiom works unchanged
  apart from the missing array syntax. An empty/unset `COMPREPLY` means no
  candidates (then `-o default` applies).
- If shish later grows arrays, `COMPREPLY`/`COMP_WORDS` become arrays and the
  scalar forms stay accepted; note it here then.

**Running shell code from the editor — the risky part** (each item is a
test):

- **Preserve state**: save and restore `$?`, `$_`, positional parameters,
  `set -e/-u/-x` flags, `LINENO`, the current `PIPESTATUS`-equivalent, the
  pending `fdstack`, and trap-in-progress; the function runs with stdout
  and stdin on `/dev/null` and **stderr discarded** (bash lets stderr
  through and messes up the line; we do not).
- **Terminal**: the editor is in raw mode; the function must run with the
  cooked attributes restored only if it wants to read (`read` in a
  completion function is not supported: stdin is `/dev/null`).
- **Runaway**: a function that loops forever hangs the prompt. ^C arrives as
  the byte 3 (raw mode), so poll the input for it between evaluated
  commands (a hook in `eval_list`, guarded by a `compl_running` flag), or
  set an `alarm()` guard (default 2 s, `COMPLETION_TIMEOUT`); either aborts
  the function and beeps.
- **Re-entrancy**: a completion function invoked while another is running
  (a `-F` calling `compgen` is fine; a TAB is not reachable) — assert with a
  `compl_running` counter.
- **`-C cmd`** runs an external program: needs `fork`; not available on WASI
  (the spec registers, generation fails with a diagnostic once).
- **Word list expansion** (`-W`): expanded when the candidates are generated,
  not when registered (bash), so `$(…)` in a word list runs on every TAB:
  documented, same trust level as `-F`. Data from the command line
  (`COMP_WORDS`, `$2`) is never `eval`ed by the engine.
- **Exit and errors**: an undefined `-F` function or a non-zero exit
  status with no `COMPREPLY` gives no candidates silently; a diagnostic only
  under `set -x`-style debug (`COMPLETION_DEBUG=1`).

**Storage**: a singly linked list of `struct compspec` in `src/complete/`
(`complete_spec_add/find/remove/print`), names lowercase-exact, strings
`str_dup`ed; a subshell sees the parent's specs and changes made there do
not leak (same treatment as functions, `exec_functions_save/restore`).

**Loading completions lazily** (optional, later): `complete -D -F _loader`
where `_loader` sources `$SHISH_COMPLETION_DIR/$1.sh` on the first TAB for
command `$1` (bash-completion's model). Ships as an example under `doc/`,
not compiled in.

### Phase 6 — usability polish (each independent, any order)

- Cycling: a second TAB with no unique prefix steps through the candidates
  (`menu-complete`), shift-TAB (`ESC [ Z`) goes back; toggle
  `COMPLETION_MENU=1`.
- `COMPLETION_IGNORE_CASE=1` (match, keep the candidate's case), a
  shell-variable substitute for readline's `completion-ignore-case`.
- Colour the list by type when `LS_COLORS`/`NO_COLOR` allow (dir, exec,
  symlink) — needs `lstat` per shown entry, only for lists ≤ the query
  threshold.
- `ESC ?` lists without inserting; `ESC *` inserts all matches (both readline).
- Paging of a long list at `LINES` rows (`--More--`).
- `WINDOWS_NATIVE`: a `FindFirstFile` source so the editor is not
  completion-less there; separator `\`, case-insensitive match.
- History-word completion (`ESC /` / `ESC .`, insert last argument) —
  belongs to the line-editor rewrite (see "Also open"), not here.

### Phase 7 — optional: exactness through the real parser (Option C)

Only if Phase 4's lexer produces wrong offers in practice (aliases expanding
to compound commands, here-docs, `case` inside `$(…)`): add `P_COMPLETE`
to the parser flags, a silent `parse_error()`, and make `parse_expect()`
record its `toks` argument and return when the cursor offset is reached;
`compl_context()` then asks the parser instead of the lexer. Measure both
against the same table before switching.

### Testing

- **Unit (no pty)**: `compgen --line 'LINE' [POS]` (dev switch, Phase 0)
  drives contexts and sources; table rows `line|pos|expected candidates`
  in `tests/term-complete.sh`, one `assert_equal` per row — hundreds of
  rows cost milliseconds.
- **Quote/escape matrix** (Phase 2): every combination of open quote ×
  special character in the name (`space ' " \ $ ` * ? [ ] ( ) ; & | < > ~ !
  newline tab`) — insert, then `eval` the result and compare with the
  file name; this is the only way to be sure the escaping is right.
- **State machine** (Phase 4): the valid-next table as a test, plus a
  random walk: generate a valid shell fragment, cut it at a random word boundary, and assert that the
  continuation's next keyword is among the offers.
- **`complete`** (Phase 5): every option, `-p` round trip
  (`complete -p | eval` reproduces the registry), `-F` with `COMP_*`,
  runaway function aborted by the guard, `$?` preserved, a `-F` that
  writes to stderr leaves the line intact (pty case), subshell isolation.
- **Editor glue** (pty, few cases): insertion, redraw after a list, cursor
  in the middle of a line (text after the cursor is kept and not
  considered), two-TAB list behaviour.
- **Fuzz**: random bytes as the line and a random cursor position under
  ASan+UBSan (the section 4 gate): `compl_words()` and `compl_context()`
  must never read outside `term_cmdline`.

### Size and cost (estimates; calibration: today's `term_complete.c` is
363 lines / ≈3 KB)

| Piece | Lines | Code |
|---|---|---|
| Phase 0-2: words, set, apply, quoting | ≈450 | ≈4 KB |
| Phase 3: contexts, sources, `$PATH` cache, defaults table | ≈500 | ≈5 KB |
| Phase 4: pending + construct stack | ≈200 | ≈2 KB |
| Phase 5: registry, `complete`/`compgen`/`compopt`, running `-F` | ≈600 | ≈6 KB (only with `ENABLE_COMPLETE`) |
| tests | ≈400 | — |
| **Total** | **≈2100 lines** | **≈11 KB without, ≈17 KB with Phase 5** |

The completer only exists in interactive builds; the `WITH_COMPLETE`
switch (default on with the line editor, off for the WASI/`-c` builds that
never read a tty) keeps a non-interactive build unchanged — verify with the
stripped-size comparison from section 15.

### Order of work (each step its own change with test and this file updated)

1. **Phase 1** first: it is the smallest visible change and needs no
   refactor (`term_complete.c` keyword list + `in` special case + tests).
2. **Phase 0**, then **Phase 2** (correctness first: quoting bugs make every
   later feature worse), then **Phase 3**.
3. **Phase 4** once Phase 3's word list exists (it reuses it).
4. **Phase 5** last of the core: everything before it is useful without it,
   and it is the one with the state-preservation risk.
5. Phases 6-7 as wanted.

### Risks and open questions

- **Arrays**: the `complete -F` protocol above is a scalar approximation.
  Existing bash completion scripts use `${COMP_WORDS[COMP_CWORD]}`,
  `COMPREPLY=( $(compgen …) )` and `local -a`; those will not parse in
  shish. Compatibility with the bash-completion package is **not** a goal;
  compatibility with the *idioms* in this section is.
- **Default builtins**: `complete`/`compgen` in `DEFAULT_BUILTINS` or
  `EXTRA_BUILTINS`? Decide from the measured size (≈6 KB, section 15.5 policy).
- **Correctness of the lexer vs. the parser**: Phase 4 accepts small drift;
  the fallback keeps it harmless. Track drift cases in `BUGS` as they are found.
- **`$PATH` scan cost** on slow/network directories: the cache is per
  directory mtime, but the first scan blocks the prompt; a soft limit
  (`COMPLETION_PATH_MAX`, default 5000 entries) and no scan of directories
  that are not `stat`-able within one call.
- **UTF-8** (section 26): candidate width in the listing and the insertion of a
  common prefix must not cut a multibyte character — use `mb_clen` when the
  common prefix is computed byte-wise.
- **Security**: `-W`/`-F` execute user-defined code on TAB; nothing typed at
  the prompt is ever `eval`ed by the engine itself. A file name that looks
  like `$(cmd)` must be inserted escaped (part of the Phase 2 matrix).
- **Interrupted long operations**: `opendir` on a hung NFS mount blocks the
  prompt; no fix planned (same in bash), note in the docs.

### How to measure

```sh
# candidate generation without a terminal (Phase 0 switch)
time (for i in 1 2 3 4 5; do build/x86_64-linux-gnu/shish -c "compgen --line 'p'" >/dev/null; done)
# $PATH scan, cold and warm
build/x86_64-linux-gnu/shish -c 'compgen --line "a" | wc -l; compgen --line "a" | wc -l'
# code size of the completer (interactive vs -c-only build), stripped
size build/x86_64-linux-gnu/CMakeFiles/libshell.dir/src/term/*.o build/x86_64-linux-gnu/CMakeFiles/libshell.dir/src/complete/*.o
```

---

## 21. Vi mode (`src/term/term_vimode.c`): gaps against vim/POSIX `set -o vi`

Tests: `tests/term-vi.sh`. `vi` is always on: there is no `set -o vi` / `set -o emacs` switch.

**Missing, roughly by how often they are used at a shell prompt**

- **Undo and repeat:** `u`, `U`, `.` (repeat last change), `Ctrl-R`-style redo (`Ctrl-r` is the history search).
- **History:** `G` / `[n]G` (go to entry n), `n` / `N` (repeat the last search), `?` (forward search; `/` today calls the emacs-style `Ctrl-R` incremental search, not a vi `/pattern<CR>` prompt), `+` / `-` (like `j` / `k`), `#` (comment the line out and enter it).
- **Editor hand-off:** `v` (open the line in `$VISUAL` / `$EDITOR`; POSIX requires it).
- **Motions:** `|` (column), `%` (matching bracket), `g_`, `ge` / `gE`, `H` / `L` / `M`, `{` / `}` / `(` / `)` (irrelevant on one line, but `d}` etc. would be used in multi-line entries), `_`, `-` / `+`.
- **Operators:** `~` (toggle case), `g~` / `gu` / `gU`, `>` / `<` (no meaning on one line), `J` (join), `!`, `=`, `gq`.
- **Text objects:** `iw` `aw` `iW` `aW`, `i"` `a"` `i'` `a'`, `i(` `a(` `ib`, `i[` `i{` `it` — `ciw`, `di"` and `ci(` are the most missed.
- **Registers and marks:** named registers (`"ayw`, `"ap`), the numbered delete history (`"1p`), `m{a-z}` / `'x` / `` `x ``; there is a single unnamed yank register only.
- **Insert mode:** `o` / `O` (meaningful only for multi-line), `gi`, `Ctrl-w` (delete word), `Ctrl-u` (delete to line start), `Ctrl-v` (literal), `Ctrl-t` / `Ctrl-d` (indent), `Ctrl-[` (as ESC), `Ctrl-o` (one command), `Ctrl-h` as backspace, `Ctrl-n` / `Ctrl-p` completion (`Tab` is the only completion).
- **Visual mode:** `v` / `V` / `Ctrl-v` and their operators; nothing exists.
- **Search inside the line:** `*` / `#` (word under the cursor), `/` and `?` within the current line.
- **Counts:** missing are counts for `i a I A` (`3ix<ESC>` repeats), for `p` / `P` (`3p`), `.` and `~`.
- **Repeat find:** `;` / `,` work, but `t` repeated with `;` does not skip past the adjacent character the way vim does when `cpo` has no `;`.
- **Pending-state display:** no `-- INSERT --` / `-- NORMAL --` indicator, no cursor-shape change (`\e[2 q` block / `\e[6 q` bar), no display of a pending operator or count.
- **Escape handling:** a lone ESC waits 50 ms for a following key (fixed timeout, not `ttimeoutlen`); there is no way to configure it.
- **Multi-line entries:** history entries with embedded newlines are shown on one line; there is no line-wise `j` / `k` / `o` / `dd` inside them.
- **Wide characters:** columns are bytes, so `l` / `x` / `w` step through a UTF-8 sequence one byte at a time.
- **Options:** none of `set -o vi`, `EDITRC` / `inputrc`-style key rebinding, `bind`, or a `vi`-mode `KEYTIMEOUT` is honoured.

---

## 22. Also open

- **WASI build (`cfg-wasi`, `doc/wasm.md`) — builds and runs (Node, webassembly.sh).** `build/wasi/shish` is 248 KB and runs under Node's WASI
  (`--experimental-wasm-exnref`): 18 of 34 `tests/*.sh` pass unmodified,
  the rest need fork/external commands or job control. Open:
  - **webassembly.sh works** (Chrome 152, upload via `wapm upload`,
    2026-09-19). The module needs WebAssembly exception handling
    (`exnref`; wasi-sdk 34 no longer emits the legacy opcodes); an older
    browser lacking it cannot load the module — the fallback would be a
    build without EH, i.e. replacing the `setjmp` unwinding in
    `src/eval/` (design-sized). The interactive prompt there is untried.
  - **Interactive use needs our own page (plan, not started).** In
    webassembly.sh `shish` works, but every stdin read is a browser
    `prompt()` dialog ("Please enter text for stdin:"), one line at a time, no
    prompt line, no editing: not usable as a shell, and shish cannot change
    that. Plan, in order:
    1. A static page: xterm.js plus a small WASI shim (`@bjorn3/browser_wasi_shim`
       or hand-written) that feeds keystrokes to fd 0 and writes fd 1/2 as they
       come, with an in-memory filesystem (writable `/tmp`, a cwd so `cd` works).
    2. The line editor: `tcgetattr` reports "not a tty" on WASI, so shish
       falls back to plain line input. Either have the page echo and edit the
       line, or let the shim answer `tcgetattr` so the editor runs in raw mode.
    3. Test it with the Chrome tools, then decide where it lives (`examples/` or
       `doc/`; the Pages site is generated from the `rsenn` repo, not here).
    Build used: `cfg-wasi` (all builtins incl. compress with xz/lzma, 1.1 MB,
    `build/wasi/shish.wasm`); tested under Node only, plus webassembly.sh `-c`.
  - External commands cannot run; a wasm-hosted "run this program" hook
    (webassembly.sh runs other `.wasm` commands by name) would need a
    host import, not an `execve`.
  - `tests/fixed.sh` hangs under WASI (job-control cases); needs a
    WASI skip list.

- **Line-editing/terminal-abstraction/key-bindings rewrite** — a
  design-sized project inherited from the old `TODO` file, not a fixable
  bug. Minimal tab-completion (`src/term/term_complete.c`: filenames, plus
  keywords, builtins and functions at a command's first word) is the
  only piece of this done so far; its growth path (context, `$PATH`,
  `complete`/`compgen`) is section 20. UTF-8-aware editing (per-character cursor and
  backspace, display columns) is milestone M5 of section 26.

---

## 23. More utilities as builtins (plan only, nothing implemented)

Two prioritized lists of programs that are not builtins yet, each with size estimates, POSIX status, whether it can
be a filter, a category, and the source file it should share with its relatives:

- `doc/coreutils.md` (58 entries from `coreutils.unimplemented`): start with the one-liners
  (`sync arch hostid yes tty nproc whoami logname printenv`, about 250 lines), then `tac truncate mkfifo nice
  nohup seq cksum`, then the text filters (`expand unexpand fold comm base64 base32`), then `chown chgrp pathchk du
  df` on the directory walker, and `dd od pr join csplit stty` last.
- `doc/util-linux.md` (80 entries from `util-linux.unimplemented`): Linux-only, mostly root-only, so every one is off
  by default behind its own `BUILTIN_<NAME>` and compiled only where the headers exist. Start with the scripting
  staples (`rev mcookie mesg setsid mountpoint fallocate flock namei isosize`), then the process wrappers
  (`chrt taskset ionice choom uclampset setarch`, `prlimit` on `ulimit`), then namespaces (`unshare nsenter pivot_root
  switch_root`, testable with `unshare -Ur`), then a shared `lib/coltab` table printer and the `ls*` listing tools.
  Not worth building: `su sulogin getty agetty fsck* mkfs.*`.

**Prerequisites, in this order** (sections 5 and 12): the builtin map so a file can hold
several builtins, the per-builtin switches (Part B) so each of them can be left out, and the directory walker (Part C)
for `chown`, `chgrp`, `du`, `hardlink` and `switch_root`. Do not start a utility from either list before sections 5 and 12 are done.

**Cross-cutting decisions to take first** (also in the "Open questions" of both documents):
- whether Linux-only builtins belong in the tree at all (the plan assumes a busybox-like single binary is the goal);
- `configure` checks and the `syscall()` fallbacks for calls without a libc wrapper (`ioprio_set`, `sched_setattr`,
  `pivot_root`), including dietlibc and musl, and keeping these files out of the Windows and wasm builds;
- tests as a normal user in a user namespace that skip themselves when the builtin or the privilege is missing.

---

## 24. More `EXTRA_BUILTINS`: POSIX utilities real scripts call most, still external

**Not started; this section is the candidate list.** The rationale is the same as for `cp`/`mv`: a
builtin is free where an external binary already exists (it only wins the `PATH` lookup +
`fork`+`exec`) and a real capability where none does (WASI, a from-scratch container,
`-DLINK_STATIC=ON` single-file image).

**Evidence.** A histogram of every command word bash actually dispatched across a large corpus of
real-world shell scripts (`../plot-cv/shell-commands-histogram.txt`, 3545 distinct names), filtered to
POSIX.1-2024 utilities that are still not shish builtins:

| Count | Utility | Count | Utility | Count | Utility |
|---|---|---|---|---|---|
| 794 | `diff` | 73 | `dd` | 12 | `nohup` |
| 412 | `tput` | 37 | `chown` | 10 | `nice` |
| 267 | `getconf` | 29 | `du` | 10 | `fold` |
| 228 | `cmp` | 26 | `od` | 8 | `tty` |
| 75 | `bc` | 20 | `df` | 6 | `mkfifo` |
| 19 | `comm` | 5 | `join` | 4 | `expand` |

(`seq`, `install`, `dir`, `yes`, `stat`, `groups`, `arch`, `stty`, `sum`, `sync`, `nproc`, `truncate`,
`fmt`, `base64`, `mknod` appear at similar frequencies but are **not** POSIX utilities, so they stay
out of scope per the "design spec is POSIX" rule.)

- **`diff`**: real value (794 uses) but by far the biggest: a line diff needs an LCS/Myers engine,
  closer in scope to `text/dfa` than to one `builtin_*.c`; give it its own `text/diff/` the way `sed`
  got `text/sed/`, and its own section if it is picked up.
- **System/identity utilities** (`chown`, `du`, `df`, `dd`, `nice`, `nohup`, `tty`): individually small
  (mostly one syscall plus formatting) but only pay off where `fork`+`exec` of the real one is
  unavailable.
- **Niche/legacy** (`getconf`, `cmp`, `bc` (non-trivial), `comm`, `fold`, `mkfifo`, `join`, `expand`,
  `od`, `pr`, `cksum`, `tsort`, `csplit`, `pathchk`, `chgrp`): candidates, not a near-term plan; no
  per-utility sizing has been done. The sized schedule for the ones in section 25 is there.
- Known gaps in the builtins that already moved: `sort` keeps everything in memory and compares bytes (no locale collation); `tail -f` follows one
  file; `split` has the POSIX options only.

No file layout, option sets, or size estimates have been worked out for any of these yet: that is the
next step once one is picked up (POSIX page -> option table -> LOC estimate -> `BUGS` entries for any
deliberately omitted option).

---

## 25. The remaining POSIX utilities as optional `EXTRA_BUILTINS`

Utilities from the POSIX utilities volume that are not builtins yet. None is needed by the shell itself,
so **this waits until the main quest is done.** Each one is opt-in (`BUILTIN_<NAME>`,
`cmake/Builtins.cmake`, off by default), lives in `src/builtin/extra/builtin_<name>.c`, follows its POSIX
page as the design specification (<https://pubs.opengroup.org/onlinepubs/9799919799/utilities/>; a
missing option is a `BUGS` entry with a repro), uses `lib/` instead of libc stdio/string, and comes
with `tests/builtin-<name>.sh` and a `help_<name>` text.

### Schedule, simplest and smallest first

Complexity: **1** one syscall or one loop, **2** a small parser or a few options, **3** real algorithm
or state, **4** terminal / `/proc` / many output formats, **5** layout engine. Lines are C lines for
the builtin alone, calibrated on what exists (`tee` 95, `mkdir` 105, `wc` 165, `ls` 274, `xargs` 319,
`touch` 326 with its date parser); tests add roughly the same again.

| # | utility | cx | lines | total | what it takes |
|---|---|---|---|---|---|
| 1 | `mkfifo` | 1 | ~45 | ~45 | `mkfifo(3)` plus the mode parser (`-m`) that `mkdir` already has; move the parser to `lib/` first |
| 2 | `nice` | 1 | ~55 | ~100 | `nice(2)`, then `exec_command()`; only the increment parsing (`-n`, and the obsolescent `-10`) |
| 3 | `nohup` | 2 | ~70 | ~170 | ignore `SIGHUP`, redirect stdout/stderr to `nohup.out` if they are terminals, `exec_command()`; exit 126/127 |
| 4 | `renice` | 2 | ~80 | ~250 | `setpriority(2)` over `-p`/`-g`/`-u` ID lists; no exec; `-n` is required in POSIX.1-2024 |
| 5 | `pathchk` | 2 | ~90 | ~340 | `pathconf(3)` limits, `-p` portable-character check, `-P` empty/leading-hyphen check |
| 6 | `du` | 3 | ~140 | ~480 | directory walk like `rm -r`; `-a -s -k -x -H -L`, hard links counted once (`st_dev`/`st_ino` set) |
| 7 | `fuser` | 4 | ~130 | ~610 | Linux only: scan `/proc/*/fd`, `cwd`, `root`, `maps`; `-c -f -u`; not portable, no test on other systems |
| 8 | `more` | 4 | ~220 | ~830 | raw-terminal pager on `src/term/` (`term_init`, window size, `SIGWINCH`); commands `q space enter /pattern`; not a filter |
| 9 | `od` | 4 | ~260 | ~1090 | `-A -j -N -t` (a c d f o u x, sizes), `-v` duplicate folding, the obsolete `-b -c -d -o -s -x` aliases, address radix |
| 10 | `ps` | 4 | ~300 | ~1390 | Linux only: `/proc/*/stat`+`cmdline`, `-o` column formats, `-e -a -A -f -l -p -t -u -U -G -g -d`, tty name mapping |
| 11 | `pr` | 5 | ~320 | ~1710 | pagination, headers/footers, `-column`/`-m` multi-column layout, `-e -i -n -o -w -f -F -l -h -s -t -d -r -a` |

**About 1,700 lines for the 11 utilities still to write** (about twice that with tests). Stop after `renice`,
or after `du`, unless a real use appears.

### Not planned

- **`rmdel`** and the other SCCS utilities (`admin delta get prs sact sccs unget val what`): SCCS is
  obsolescent and dropped from newer POSIX editions. `rmdel` alone would need the whole SCCS file
  format (`admin`, `get`, `delta`, weave parsing): **1,500+ lines** for a version-control system
  nobody uses. Out of scope, like `lex`, `yacc` and `c99` (`CLAUDE.md`, "Design specification for
  builtin utilities").

### Shared code to extract first

Do these two before the utilities that need them; each removes a duplicate:

1. **mode-string parser** (`chmod`, `mkdir`, `mkfifo`): now inside `builtin_chmod.c`/`builtin_mkdir.c`, move to `lib/`.
2. **wrapper helper** for "set something up, then `exec_command()` the rest" (`env`, `nice`, `nohup`, and
   `xargs`/`timeout` already do it by hand): about 30 lines around `exec_hash()` + `exec_command()`.

Filters (`head uniq paste cut tr nl tail`, already builtins) use `src/builtin/builtin_filter.[hc]` (`filter_in`,
`filter_ops`: declarative `opts/size/option/setup/step/finish`, see the comment in the header) so they can join filter chains (section 19); `tail -f` and `more` cannot chain.

---

## 26. Optional UTF-8 support (`WITH_UTF8`, off by default)

**Not started; this section is the plan.** shish is byte-oriented and
never calls `setlocale`, i.e. it is always in the POSIX locale, which is
what POSIX requires (bytes, C locale: section 10). This goal adds *character*
semantics for a UTF-8 locale (`LC_ALL=C.UTF-8`) as a compile-time option;
the default build stays C-locale-only and must not grow.

### What "UTF-8 as POSIX specifies it" means for a shell

In a locale whose charmap is UTF-8 a *character* is one UTF-8 sequence.
The places where POSIX makes the shell care:

- `${#param}` is a length in characters. (Measured 2026-09-19 with
  `x=é; echo ${#x}`: bash, zsh, ksh, busybox ash give 1 under
  `LC_ALL=C.UTF-8`; dash gives 2; shish gives 2.)
- Pattern matching — `?`, `[...]` members and ranges, and the stepping of
  `*` — in `case`, `${p#pat}`/`${p%pat}`/`##`/`%%`, and pathname expansion.
- IFS characters (field splitting, `"$*"`'s first-*character*-of-IFS
  separator, `read`). POSIX 2.6.5 defines IFS in terms of "the byte
  sequences that form the characters", so a multibyte IFS character is
  one separator, not several.
- Utilities' own character semantics: `wc -m`/`-L`, `expr :` and the regex
  utilities (section 10). **`printf` is not in this list**: POSIX.1-2024
  `printf` says `%c` prints the *first byte* of the argument and has no
  multibyte provision for `%s` precision either (bytes), so it stays
  bytewise (verified 2026-09-19); bash's character behaviour is an
  extension, not a model.
- The interactive line editor: cursor keys, backspace and redraw are per
  character, with display columns ≠ characters for wide/combining ones.
- Locale switching: `LC_ALL`, `LC_CTYPE`, `LANG` are shell variables whose
  assignment changes the shell's own behaviour.

What does **not** change: lexing, parsing, quoting, names, arithmetic,
redirections, job control. Every non-ASCII byte in UTF-8 is ≥ 0x80, so a
multibyte character never contains an ASCII shell metacharacter, and the
`\`-escape marker (one byte) protecting a lead byte is harmless (add a
test: `echo \é '*é*'`). Nothing in `lib/buffer`, `lib/stralloc`,
`lib/str`, `lib/byte` needs to change either: they are byte containers,
and `str_len()`/`byte_*` stay the right thing for sizes and allocation.

**So it is not only `${#x}` and range indexing**, but it is also not a
rewrite. Seven areas, listed by how invasive they are:

| # | Area | Sites | Invasiveness |
|---|---|---|---|
| A | language-visible expansions | `expand_param.c` (7 sites), `"$*"` separator | small |
| B | pattern matching core | `lib/path/path_fnmatch.c` (one function serves `case`, `%`/`#`, and our regex bracket sets) | moderate, one file |
| C | pathname expansion | `wordlist_glob.c` calls the **libc** `glob()` | policy decision (below) |
| D | IFS with non-ASCII characters | `wordlist_cat.c`, `wordlist_glob.c`, `builtin_read.c` | small, rare case |
| E | builtins | `builtin_printf.c`, `builtin_wc.c`, `builtin_expr.c`/`lib/dfa` | small each |
| F | line editor | `src/term/*` (906 lines, ~10 files) | **largest** |
| G | locale detection/switching | `sh_init.c`, `var_setsa.c` (the `RANDOM` special-case is the precedent) | small |

### Compile-time and runtime switches

- `src/features.h` already carries `WITH_*` toggles (`WITH_PARAM_RANGE`);
  add `WITH_UTF8` (default 0). CMake: `option(ENABLE_UTF8 ... OFF)` →
  `-DWITH_UTF8=1`; autotools: `--enable-utf8`.
- Runtime: even a `WITH_UTF8=1` binary is byte-wise unless the locale
  says otherwise. `mb_utf8` is 1 iff the effective `LC_CTYPE` (POSIX
  order: `LC_ALL`, else `LC_CTYPE`, else `LANG`) has a `.UTF-8`/`.utf8`
  charmap suffix, case-insensitive. shish decides this itself from the
  variables; it does **not** need libc's `setlocale` for its own logic,
  so it also works on dietlibc and mingw.
- Hooks: `sh_init()` (initial environment) and `var_setsa.c`/`var_unset.c`
  when the name is one of the three (same shape as the `RANDOM` case at
  `var_setsa.c:36`). **Risk:** temporary command-prefix assignments
  (`LC_ALL=C.UTF-8 wc -m`) may bypass `var_setsa`; verify with a test
  before trusting the hook.

### API (`lib/mb.h`, prefix `mb_`) — shaped like the byte code it replaces

Design rule: the non-UTF-8 code is the reference; the API is bent to fit
it, not the other way round. Two consequences:

1. Step functions return a **byte length to add/subtract**, so an
   existing byte loop keeps its shape: `i++` becomes
   `i += mb_clen(v + i, vlen - i)`, `i--` becomes `i -= mb_plen(v, i)`.
   In a byte build those are the constants `1`, and the compiler emits
   the same code as before.
2. There is **one** `#if WITH_UTF8` — in `lib/mb.h`, whose `#else` branch
   defines every function as a byte version (`static inline`, arguments
   evaluated once). Call sites carry no `#ifdef`; only code whose logic is
   structurally different in UTF-8 (bracket-member decoding in
   `path_fnmatch`, multibyte input assembly in the editor) gets an
   explicit `#if WITH_UTF8` block next to its byte version.

```c
#if WITH_UTF8
extern int mb_utf8;                                   /* 1 while the locale is UTF-8 */
void   mb_locale(void);                               /* re-read LC_ALL / LC_CTYPE / LANG */
size_t mb_clen(const char* s, size_t n);              /* bytes of the char at s; never 0 (n == 0 gives 1) */
size_t mb_plen(const char* s, size_t pos);            /* bytes of the char ending at s+pos; pos > 0 */
size_t mb_len (const char* s, size_t n);              /* characters in n bytes */
size_t mb_off (const char* s, size_t n, size_t chars);/* byte offset after `chars` characters, clamped to n */
size_t mb_cols(const char* s, size_t n);              /* display columns of n bytes (optional: needs width table) */
size_t mb_get (const char* s, size_t n, unsigned long* cp); /* decode: bytes consumed (>= 1), code point in *cp */
#else  /* byte versions: 1, 1, n, min(chars, n), n, (*cp = (unsigned char)*s, 1) */
#endif
```

`mb_clen` never returns 0 because the existing suffix loops visit
`i == vlen` (the empty suffix) and rely on stepping past it. Every
`mb_*` function starts with `if(!mb_utf8)` and returns the byte answer,
so a `WITH_UTF8=1` binary under `LC_ALL=C` behaves identically to the
default build. `mb_put` (encode) is deliberately left out until
something needs it (`printf '\u'` is not POSIX).

Policies (decide once, test once):

- Invalid, overlong, truncated-at-end-of-buffer, surrogate (D800-DFFF)
  or > U+10FFFF sequences: **each offending byte is one character**
  (width 1, code point = the byte value). Matches what bash does.
- A combining mark is a character of its own (`${#x}` counts it), as in
  bash; no grapheme clustering.
- Ordering (`ls`, glob results) stays byte order; for valid UTF-8 that
  equals code-point order, which is what `C.UTF-8` collation is (glibc
  documents this; verify). No `strcoll`.
- Non-ASCII `[:alpha:]`/`[:upper:]`… : **out of scope for the first pass**,
  classes match ASCII only (documented deviation). Optional later, only
  where libc can answer (`iswctype` after `setlocale`): glibc and musl.

### What the spec text says (POSIX.1-2024, read 2026-09-19)

| Statement | Where | Consequence here |
|---|---|---|
| `${#parameter}`: "the length in characters of the value of parameter" | 2.6.2 | `mb_len` in a UTF-8 locale; bytes in POSIX locale |
| IFS: "byte sequences that form the characters" | 2.6.5 | a multibyte IFS character is one delimiter |
| `"$*"`: fields joined by "the first character of IFS" | 2.5.2 | `mb_clen(ifs)` bytes, not `ifs[0]` |
| pattern matching: `?` one *character*, bracket members are characters | 2.14 | area B |
| POSIX locale = 256 single-byte characters, UTF-8 never mandated | 6.2 | a bytes-only shish is conformant; this whole goal is optional |
| `printf %c`: "first byte", no multibyte text for `%s` precision | printf | **no change** (see risks) |
| `wc -m`: number of characters; `-c` bytes | wc | area E |
| `LC_ALL` > `LC_CTYPE` > `LANG` precedence, null = unset | 8.2 | `mb_locale()` |

### `mb_*` algorithms (no libc, no `wchar_t`)

**What is a valid sequence** — the Unicode "well-formed UTF-8" table; the
whole validator fits in one `switch` and needs no `scan_utf8`:

| First byte | Length | 2nd byte | 3rd, 4th |
|---|---|---|---|
| 00-7F | 1 | — | — |
| C2-DF | 2 | 80-BF | — |
| E0 | 3 | **A0**-BF | 80-BF |
| E1-EC, EE-EF | 3 | 80-BF | 80-BF |
| ED | 3 | 80-**9F** (no surrogates) | 80-BF |
| F0 | 4 | **90**-BF (no overlong) | 80-BF ×2 |
| F1-F3 | 4 | 80-BF | 80-BF ×2 |
| F4 | 4 | 80-**8F** (≤ U+10FFFF) | 80-BF ×2 |
| 80-BF, C0, C1, F5-FF | invalid | — | — |

`mb_clen(s, n)`: if `!mb_utf8 || n == 0 || s[0] < 0x80` return 1; look up
the row; if fewer than `len` bytes remain or a continuation byte is out
of its range, return 1 (**each byte of an invalid or truncated sequence is
its own character**); else return `len`. Noncharacters (U+FFFE, U+FDD0..)
are valid, like glibc. This makes `scan_utf8` (which accepts surrogates
and > U+10FFFF, see below) unnecessary for lengths; it is only useful for
`mb_get`'s code-point value, and a 15-line shift/or over the validated
bytes does that too — so **no c-utils code has to be imported** unless
`fmt_utf8` (encoding) or the width table is wanted.

`mb_plen(s, pos)` (character ending at `s+pos`, `pos > 0`): `k = pos-1`;
while `k > 0 && pos-k < 4 && (s[k] & 0xC0) == 0x80` do `k--`; if
`mb_clen(s+k, pos-k) == pos-k` return `pos-k`, else 1. (`n = pos-k`
makes a sequence that would extend past `pos` count as truncated.)

`mb_len(s, n)`: `for(i = c = 0; i < n; c++) i += mb_clen(s+i, n-i);`
`mb_off(s, n, chars)`: same loop stopping after `chars` steps, clamped to `n`.
`mb_cols(s, n)`: sum of `wc_charwidth(cp)` per character, 1 for invalid
bytes, 0 for combining marks (M6 only).

**Property tests** (`tests/dev/mb-fuzz.c`, dev-only, ASan+UBSan, before
M2 lands): for random byte strings up to 64 bytes over the alphabet
{ASCII, 80, 8F, 9F, A0, BF, C0, C2, E0, E2, ED, F0, F4, F5, FF} (dense in
boundary cases): (1) walking forward with `mb_clen` and walking backward
with `mb_plen` give the **same boundaries**; (2) `mb_len` equals the
number of forward steps; (3) `mb_off(s, n, mb_len(s, n)) == n`; (4) each
step is ≥ 1 and never passes `n`; (5) for strings that are valid
UTF-8 (per `iconv -f UTF-8 -t UTF-8` on the host, or Python's decoder in
the driver script) `mb_len` equals the code-point count; (6) with
`mb_utf8 = 0` every function returns the byte answer.

**Locale detection** (`mb_locale()`): value = `LC_ALL` if set and non-empty,
else `LC_CTYPE` if set and non-empty, else `LANG` if set and non-empty,
else `""`. Format is `language[_territory][.codeset][@modifier]`: take
the text after the first `.` up to `@` or the end, remove `-` and `_`,
compare case-insensitively with `utf8`. Table: `C.UTF-8`, `en_US.utf8`,
`de_DE.UTF-8@euro` → on; `C`, `POSIX`, `""`, `en_US`, `en_US.ISO-8859-1`,
`en_US.UTF-16` → off. Cost: three `var_get` calls, done only when one of
the three variables changes (hook) and once at start-up. No `setlocale`.

### Per-site sketches (byte build = the existing code, unchanged)

`expand_param.c` — the removal loops already index bytes in `v[0..vlen]`;
only the step changes:

```c
/* %  (smallest suffix): today  for(i = vlen - 1; i >= 0; i--) */
for(i = vlen - mb_plen(v, vlen); ; i -= mb_plen(v, i)) {   /* mb_plen(v,0) not called: */
  if(path_fnmatch(sa.s, sa.len, v + i, vlen - i, 0) == 0) break;
  if(i == 0) { i = -1; break; }                            /* keeps the "no match" result */
}
/* %% (largest suffix): today  for(i = 0; i <= vlen; i++) */
for(i = 0; i <= vlen; i += mb_clen(v + i, vlen - i))       /* clen(…,0) == 1 steps past the end */
/* #  (smallest prefix): today  for(i = 1; i <= vlen; i++) */
for(i = mb_clen(v, vlen); i <= vlen; i += mb_clen(v + i, vlen - i))
/* ## (largest prefix):  today  for(i = vlen; i > 0; i--) */
for(i = vlen; i > 0; i -= mb_plen(v, i))
```

`S_STRLEN`: `wordlist_cat(wl, lstr, fmt_ulong(lstr, mb_len(v, vlen)), …)`.
`S_RANGE` (`${v:off:len}`, characters like bash): `nc = mb_len(v, vlen);
r = limit(&r, 0, nc); o = mb_off(v, vlen, r.offset);
e = o + mb_off(v + o, vlen - o, r.length);` then `wordlist_cat(wl, v + o, e - o, …)`.
`"$*"`: `sep = ifs[0] ? mb_clen(ifs, str_len(ifs)) : 0` and append
`sep` bytes (0 = no separator: the fix for `BUGS: star-empty-ifs-appends-nul`,
which must land first).

`path_fnmatch.c` — three changes, each behind the shared `mb_*` names:
`?` consumes `mb_clen(str, len)` bytes; the `*` backtrack advances the
subject start by `mb_clen` instead of 1 (so it never resumes inside a
character); a bracket member/range endpoint is read with `mb_get` from
the pattern and the subject character is read with `mb_get`, then
`lo <= cp && cp <= hi` (in a byte build the same comparison happens on
`unsigned char`, which is exactly today's code). Literal runs stay a
byte compare — correct because UTF-8 is a prefix code. `[[:class:]]`
uses the ASCII tables for cp < 0x80 and *never matches* above it (first
pass). An invalid byte in the subject matches only `?`, `*`, a negated
bracket or itself (as a single "character" with cp = byte value ≥ 0x80
outside any valid range, so it cannot be matched by `[à-ÿ]`: use cp =
0x110000 + byte for invalid bytes to keep it out of every real range).

`builtin_wc.c`: `-m` = `mb_len` of the buffer, carried across read chunks
(a character may straddle two reads: keep up to 3 bytes of carry, the
simplest correct form is counting non-continuation bytes for valid
input and falling back to `mb_len` per chunk otherwise; write the
straightforward carry version and test with 1-byte reads).

`builtin_read.c` / `wordlist_cat.c` / `wordlist_glob.c` (IFS, M6): today
IFS is a byte set queried with `str_chr`/`scan_charsetnskip`. With
non-ASCII IFS characters: build once per split a small array of
`(ptr, len)` characters from IFS; a position is a delimiter when
`mb_clen` bytes at that position equal one of them. All-ASCII IFS keeps
the byte set path (the common case, zero cost).

### Tests (`tests/utf8.sh`, written first — M0)

Run with the binary's own `LC_ALL=C.UTF-8` (set inside the script, as a
plain assignment and as a command prefix — the hook risk above) and with
`LC_ALL=C`. One assertion per line, "byte build" expectation in the
comment. Cases (`é` = C3 A9, `€` = E2 82 AC, `😀` = F0 9F 98 80, and
`\351` = a lone byte E9, invalid):
- `x=é; echo ${#x}` → 1 (UTF-8) / 2 (C); `x=€😀; echo ${#x}` → 2 / 7;
  `x=a\351b; echo ${#x}` → 3 / 3 (invalid byte = one character);
  `x=; echo ${#x}` → 0.
- `x=éa; echo ${x#?}` → `a` / `\251a` (C: strips one byte);
  `${x%?}`, `${x##*}`, `${x%%?}`, `${x#é}`, `${x#[é]}`, `${x#[à-ÿ]}`.
- `case é in ?) …` → match / no match; `case é in [é]`, `[a-z]` (no),
  `[à-ÿ]` (yes), `[!a]` (yes), `case €x in ?x)`, `case '*é' in \*?)`.
- `${x:0:1}`, `${x:1}`, `${x: -1}` on `éa€` (only if `WITH_PARAM_RANGE`).
- `set -- a b c; IFS=é; echo "$*"` → `aéb…` (separator = whole char);
  `IFS=€ read a b <<<` … splits at the 3-byte char; IFS=`é` and input
  containing byte A9 alone must not split.
- `echo \é '*é*'`, `echo "é"`, names `é=1` must still be an error
  (identifiers stay ASCII), redirect to a file called `é`.
- `printf '%c' é | od -c` → **one byte C3** in both builds (POSIX);
  `printf '%.1s' é` → C3.
- `wc -m` and `wc -c` on `é€😀` → 3 and 9 (UTF-8) / 9 and 9 (C).
- Line editor (M5): scripted through the pty helper in
  `tests/pty-run` — type `é`, press Backspace, expect an empty line; type
  `aé`, Left, Left, `X` → `Xaé`; a partial sequence written in two writes
  (C3, then A9) must produce one `é`.

### Sites (byte behaviour in the right column must stay exactly as today)

| File:line | Today | UTF-8 change | Est. lines |
|---|---|---|---|
| `expand/expand_param.c:265` `S_STRLEN` | `fmt_ulong(vlen)` | `mb_len(v, vlen)` | 2 |
| `:352` `S_RSSFX` (`%`) | `i` from `vlen-1` down by 1 | `i -= mb_plen(v, i)` | 4 |
| `:370` `S_RLSFX` (`%%`) | `i++` up to `vlen` | `i += mb_clen(v+i, vlen-i)` | 3 |
| `:388` `S_RSPFX` (`#`) | `i++` | same | 3 |
| `:409` `S_RLPFX` (`##`) | `i--` | `i -= mb_plen(v, i)` | 3 |
| `:423` `S_RANGE` (`${v:off:len}`, `WITH_PARAM_RANGE`) | byte offset/length | offsets are characters: `mb_off` twice | 6 |
| `expand_param.c` (`$*`), `wordlist_glob.c` | `"$*"` joins with `ifs[0]` | `mb_clen(ifs, ifslen)` bytes | 4 |
| `lib/path/path_fnmatch.c` | `?`, `*` backtrack, bracket members/ranges by byte | `?` and star step by `mb_clen`; bracket members decoded with `mb_get` (`[éü]`, `[à-ÿ]` by code point); literal runs unchanged (byte compare is correct) | 40-60 |
| `wordlist/wordlist_glob.c` | libc `glob()` | see "libc" below | 5-10 |
| `wordlist_cat.c`, `wordlist_glob.c`, `builtin_read.c` | IFS as a *byte* set (`str_chr`, `scan_charsetnskip`) | only matters when IFS contains non-ASCII: add `mb_span`/`mb_cspan`. **P3**, known gap until done | 30 |
| `builtin_printf.c:308-345` | `%s` width/precision and `%c` in bytes | **none** — POSIX says bytes (see above) | 0 |
| `builtin_wc.c` | `-m` = bytes; `-L` = bytes | `-m`: `mb_len` (one per character; invalid byte = 1); `-L`: `mb_cols` | 20 |
| `builtin_expr.c`, `lib/dfa` | bytes | `length`/`index`; regex via `DFA_UTF8` (section 10) | later |
| `src/term/*` (below) | column = byte index | see below | 100-150 |
| `sh/sh_init.c`, `var/var_setsa.c`, `var/var_unset.c` | — | `mb_locale()` calls | 10 |
| `parse/*`, `source/*` error columns | byte column | cosmetic, **not planned** | 0 |
| `getopts` / `lib/shell/shell_getopt.c` | one byte per option | multibyte option letters: **not planned** | 0 |

**Line editor (F).** `term_pos` is at once the byte index into
`term_cmdline` and the number of columns, and `term_left(n)` emits `n`
backspaces. Least invasive fix: keep `term_pos` a **byte** offset and
make the cursor-motion primitives convert bytes to columns —
`term_left(bytes)`/`term_right(bytes)` compute `mb_cols(slice)` and emit
that many `\b`/`CSI D`. Callers that already work in bytes stay as they
are. Only the "one character" operations change: `term_backspace`
(remove `mb_plen` bytes), `term_delete` (`mb_clen`), the `C`/`D` arrow
handling in `term_ansi.c` (count characters via `mb_off`),
`term_insertc` (takes a whole character: `term_read.c:188-190` must
buffer a partial sequence until complete), and `term_complete.c`
(common-prefix truncation at a character boundary; per-char insertion).
Without this, a UTF-8 build corrupts the command line on the first
backspace over a multibyte character, so **M5 gates enabling `WITH_UTF8`
for interactive use**. There is no prompt-width tracking today, so
nothing to change in `prompt/`.

**libc (C).** Pathname expansion is the platform's `glob()`
(`wordlist_glob.c`; `lib/unix/glob.c` is Windows-only), and glibc/musl
`glob`/`fnmatch` follow the locale set with `setlocale`, which shish
never calls. Options: (a) leave it — `?` in a *pathname* pattern stays
bytewise, everything else is right (documented gap); (b) under
`HAVE_SETLOCALE`, call `setlocale(LC_CTYPE, "")` when `mb_utf8` turns on
so libc agrees (glibc, musl); dietlibc/mingw stay at (a). Start with (a),
add (b) as its own step. The internal `glob` planned in section 15.3
(`USE_LIBC_GLOB=OFF`) removes the problem for static builds without
`setlocale`: it matches with `path_fnmatch`, which M3 makes UTF-8-aware.

### What was found in the existing UTF-8 code (looked at only after the design above)

shish's own `lib/utf8/` (8 files, `u8len`, `u8towc`, `wctou8`, …) is
**unused** anywhere in `src/` or `lib/` — flagged, not to be removed
unasked. It is not a base for this: `u8len` trusts the lead byte and
never checks continuation bytes, `u8towc` does not reject overlong forms,
and the API is built on `wchar_t`, which is 16-bit on mingw (a supported
target), so code points above U+FFFF do not fit.

`../c-utils/lib` (tried `utf8/`, `ucs/`, `scan/scan_utf8*.c`,
`fmt/fmt_utf8.c`):

| Function | Verdict |
|---|---|
| `scan_utf8(in, len, &cp)` (85 lines) | **use.** Decodes strictly: rejects overlong, stray continuation, truncated, `0xFE`/`0xFF`. Tested 2026-09-19: **accepts** surrogates (`ED A0 80` → U+D800), values above U+10FFFF (`F4 90 80 80`) and 5-byte forms (`F8 88 80 80 80`), so `mb_get` must add `cp <= 0x10FFFF` and a surrogate check (2 lines). Returns 0 on invalid → policy above (one byte, one character). Independent of `wchar_t`. |
| `scan_utf8_sem` | no: also rejects the noncharacters `U+FFFE/FFFF`, `U+FDD0-FDEF`, which are valid scalar values glibc accepts |
| `fmt_utf8(dest, cp)` (35 lines) | keep in reserve for `mb_put` |
| `wc_charwidth(wchar_t)` (61 lines, Kuhn's wcwidth) | **optional**, for `mb_cols`. Linear scan over ~150 range pairs (fine for an editor), Unicode 5-era tables (newer wide emoji are missing), `wchar_t` argument (change to `uint32`). ≈1.5 KB of tables. |
| `u8b_len` | no: counts an invalid byte as **6** (HTML-entity width), shell needs 1 |
| `u8_len` | no: returns 0 for both NUL and invalid, trusts the lead byte |
| `u8b_chrs`, `u8b_rchrs`, `u8*_diff`, `u8s_*` | not needed (search/compare helpers) |
| `wcs_to_u8s`, `u8_to_wc`, … | no: `wchar_t` |
| `ucs/*` (Latin-1 conversion) | not relevant |

Plan (revised after writing the algorithms above): `mb_clen` embeds the
Unicode well-formedness table itself (one `switch`, no surrogate/range
gap to patch), so `scan_utf8.c` is **not needed** for lengths; copy it
only if `mb_get` should reuse its decoder. `fmt_utf8.c` and
`wc_charwidth.c` are copied later, only if `mb_put`/`mb_cols` are built.

### Size estimate (estimates, nothing written)

| Piece | Lines | Code, `WITH_UTF8=1` |
|---|---|---|
| `lib/mb.h` + `lib/mb/` (`mb_locale`, `mb_clen`, `mb_plen`, `mb_len`, `mb_off`, `mb_get`, `mb_cols`) | ≈250 | ≈2-3 KB |
| copied later, optional: `fmt_utf8` 35, width table 61 (`scan_utf8` no longer needed) | ≈100 | ≈0.3 KB + ≈1.5 KB tables |
| call-site edits in A, B, D, E, G | ≈150-200 | ≈1-1.5 KB |
| editor (F) | ≈100-150 | ≈1 KB |
| tests (`tests/utf8.sh`) | ≈200 | — |
| **Total** | **≈500-650 lines** | **≈4-6 KB (+1.5 KB with width tables)** |

**Default build (`WITH_UTF8=0`): 0 bytes** — that is a requirement, not a
hope: compare the stripped `shish` size before/after each milestone
(section 15's tooling) and treat any growth beyond a few bytes of noise as a
bug in the `#else` branch.

### Order of work (each step its own change with test, `fixes/NN` where it fixes something, and this file updated)

- **M0** — `tests/utf8.sh` with the oracle table (bash's answers; dash
  as the byte reference): it must pass in *both* builds, asserting the
  C-locale answers when `WITH_UTF8=0` **even with `LC_ALL=C.UTF-8`
  exported** (that is the "tiny C-only binary" contract).
- **M1** — `lib/mb` + `mb_locale()` + hooks (G). `${#x}` alone flips.
- **M2** — area A: `${#}`, the four removal forms, `${v:off:len}`,
  `"$*"` separator.
- **M3** — area B: `path_fnmatch` (also fixes `case` and `%`/`#`
  pattern semantics in one place).
- **M4** — area E: `wc` (`printf` stays bytes), then C(b) `setlocale` if wanted.
- **M5** — area F: line editor. Gate for interactive `WITH_UTF8`.
- **M6** — area D (non-ASCII IFS), width table for wide characters.
- **M7** — section 10's `DFA_UTF8`: character sets as code-point ranges,
  compiled to UTF-8 byte-sequence automata so the DFA stays byte-based
  (the 256-bit bracket sets in section 10 cover the ASCII part only).

### Risks and open questions

- Character classes for non-ASCII stay ASCII-only in the first pass;
  say so in `--help`/docs, or it will be reported as a bug.
- `LC_ALL=C.UTF-8` as a command-prefix assignment: see the hook risk above.
- `${v:off:len}` is a non-POSIX extension already on by default
  (`WITH_PARAM_RANGE`); its UTF-8 behaviour follows bash (characters).
- Invalid input: nothing may crash or loop — fuzz the `mb_*` functions
  with random bytes under ASan+UBSan (memory-safety gate, section 4) before M2 lands.
- `printf` `%c`/`%.Ns` stay bytes (POSIX.1-2024); a later "bash
  compatibility" request to make them characters is a scope change, not a bug.
- `"$*"` with an empty IFS is broken independently of UTF-8
  (`BUGS: star-empty-ifs-appends-nul`); fix it before touching that
  line, or M2's `mb_clen(ifs, …)` would return 1 for the NUL terminator.

---

## 27. Variables: scopes, lookup and memory; and what `exec_hash` really costs (plan)

Evidence first (callgrind, MinSizeRel + all builtins, `tests/wordlist/frag.sh`-style script: 400 live variables,
3000 iterations of expansions, assignments, `set --`, `for`, `case`, `$(...)`; and a loop calling a function with a
`local`). Nothing below is implemented.

**What `src/var*` and `src/vartab*` are today** (1526 lines of C in 37 files).
- A scope is `struct vartab`: 64 bucket lists (536 bytes, zeroed by every `vartab_push`: function call, prefix
  assignment, `$(...)`, subshell, pipeline stage, `env`). Pop walks all 64 buckets.
- A lookup (`var_search`) hashes the name twice (`var_rndhash`, a 4-round mixer, picks the bucket and filters;
  `var_lexhash` packs 6 bits per character so lists stay sorted) and then probes the bucket of every scope from the
  innermost to the root. Names up to 10 characters are matched by `lexhash` alone, longer ones by `var_bsearch`.
- A second, global list `var_list` is kept sorted by `lexhash`; a new name is inserted by walking it. `var_export`
  and `var_count` walk it too, and `exec_program` calls both for every external command (two O(n) walks).
- A variable is a 128-byte `struct var` (4 list pointers, 2 hashes, 3 scope pointers, a `call` pointer) plus a separate
  `name=value` buffer from `stralloc` (at least 31 bytes): two mallocs, **208 bytes per variable** measured with
  20000 variables. A shadowing variable borrows its parent's string (`sa.a == 0`) until first written.
- Good and to keep: the `name=value` layout (envp points straight into it), buffer reuse on reassignment, borrowed
  strings for imported environment variables, `V_LOCAL`/`function` scope rules (non-local assignments skip the
  transient function scope; a `V_LOCAL` variable found in any scope is reused).

**Measured costs.**

| | instructions |
| --- | --- |
| `vartab_hash` per lookup | 187 (`rndhash` 122, `lexhash` 41) |
| `var_search` per lookup, all in | about 307 |
| lookups, share of the whole run | 17.5% (about 63 lookups per loop iteration) |
| `var_vdefault` calls | 88000 in 3000 iterations, nearly all `IFS` (one per simple command, `for`, `$*`); 10% of the run |
| `vartab_cleanup` per function call | about 800 (the 64-bucket walk) |
| function call scope, all in (`push` + `create` + `add` + `pop`) | about 17% of a call loop |

**Findings.**
1. Hashing is about 60% of a lookup, and the scope-chain walk is the rest. A plain FNV-1a plus one open-addressing
   probe is about 40 to 60 instructions.
2. `IFS` is looked up through the whole machinery once per command. It only changes when `IFS` is created, assigned,
   unset or its scope pops.
3. The sort order is only observable in `set`, `export -p`, `readonly -p`, and the `local` listing. Keeping two sorted
   structures current on every insert to serve those is the wrong way round: sort the n pointers when printing.
4. The per-scope 64-bucket table is 536 bytes where most scopes hold zero to three variables.
5. `var_unset` leaks the `struct var` (`BUGS: unset-leaks-the-var-node`, 144 bytes per cycle).
6. `exec_hash` pays for `PATH` staleness on every command (below), and `var_create` already calls
   `exec_hash_invalidate_all()` for any `PATH` assignment, so the per-command comparison is only needed for the case
   where a scope pop restores an older `PATH` (`PATH=/x cmd`, `local PATH`); the epoch in step 2 covers that one too.
   `exec_hash` also hashes the name twice (`exec_hash` and `exec_lookup`), and `builtin_search` is a linear scan of
   the table (three passes on a cache miss, once per name).
7. Dead weight: `V_CALL` and the `call` member (never set anywhere), `extern var_exported` (never defined),
   `var_dump`/`vartab_dump` (only the `dump` builtin), `rndhash`/`lexhash`/`hsearch`/`bsearch` once the table is
   replaced.

**Target design.**
```
global table   open addressing, power-of-two mask, FNV-1a, slot = struct var* (the innermost binding of that name);
               the key is the name inside var->sa.s (borrowed, no copy); grows at 70%
struct var     { sa (name=value), len, offset, flags, parent (the binding it shadows), next (same scope) }  ~72 bytes
struct vartab  { parent, level, function, struct var* vars }                                                 ~32 bytes
push           O(1): link the vartab, nothing zeroed
lookup         one hash, one probe; no walk over scopes, no per-scope table
create         in scope S: new var, parent = slot's old var, slot = new var, link into S->vars
pop            for each var in S->vars: slot = var->parent (or erase); release the node
print          collect pointers, sort by name, print            (set, export -p, readonly -p)
export         iterate the table; count kept in a counter, so exec_program needs no var_count walk
watch bit      V_WATCH on PATH and IFS: any create/set/unset/pop touching one bumps var_epoch (an int)
```
Semantics that must survive (each has a test or needs one first): shadow borrows the parent's string until written;
`READONLY` is inherited by the shadow; `V_LOCAL` reuse across scopes; `unset` removes every level of the name;
`V_INIT`; `RANDOM`; imported environment variables use caller-provided nodes and borrowed strings; the five
longjmp landings that pop scopes (`sh_pop`, `eval_pop`, `eval_jump`, `eval_return`, `eval_exit`, all
`while(varstack != saved) vartab_pop(varstack)`, which still works unchanged); `struct env` snapshots `varstack` for
subshells.

**Arena (what fits and what does not).**
- Values and root-scope nodes: no. They are freed in any order and grow; the criteria of section 13 exclude them.
- Nodes of non-root scopes: yes. Their lifetime is the scope, so a `var_arena` is told at `vartab_push` and rewound
  at `vartab_pop`: function-call, prefix-assignment and subshell scopes cost no malloc and no free. The pop must stay
  in step with the five landing sites above (it already is: they all call `vartab_pop`).
- Strings of scope variables: a prefix assignment (`FOO=bar cmd`) and `local x=1` write once, so an exact-size arena
  string fits; the catch is growth (`s="$s x"` in a function loop would leave a hole per append until the function
  returns). Rule: a scope string lives in the arena only until it is first reassigned to a longer value, then it moves
  to the heap (`V_ARENASTR` bit; the five writers of `var->sa` are `var_setv`, `var_setvsa`, `var_set`, `var_setsa`,
  `var_copys`). Measure before doing this part; it may not pay.
- Root-scope nodes: a same-size free list or slab chunk instead of `alloc()` removes the per-node header and keeps
  the nodes together; decide after the new table is in and `mallinfo` is re-measured.

**Order of work** (each step its own commit; `tests/fixed.sh` 5 known failures, `expand-fields.sh`, ctest, yash suite
and ASan+UBSan as for section 17; `size shish` must not grow):
1. Fix `unset-leaks-the-var-node`; delete `V_CALL`, `call`, `var_exported`. Tiny, independent.
2. `var_epoch` and `V_WATCH` for `PATH` and `IFS` on the *current* structure: `wordlist_init` callers read a cached
   `IFS` pointer, `exec_hash` compares an epoch instead of `var_value("PATH")` plus `strcmp` (see below). Expected:
   about 8% of the instructions in the workload above from `IFS`, about 6% from `PATH`.
3. The new table and scope list (replaces `vartab_*`, `var_search`, `var_hsearch`, `var_bsearch`, `var_lexhash`,
   `var_rndhash`; adds the sorted print). Expected: lookups from about 307 to about 60 instructions, `struct var`
   128 to about 72 bytes, `struct vartab` 536 to about 32, `vartab_cleanup` from 800 instructions to a few per
   variable; fewer lines (the two hashes and both searches are about 250 of the 1526).
4. Scope arena for nodes, then (if measured worth it) for strings.
5. `exec_program`: drop the `var_count` walk (count kept by step 3).
Measure each step with `tests/wordlist/frag.sh` + `mallinfo.c`, a function-call loop, and callgrind; keep the numbers
in this section.

**`exec_hash` with `lib/hashmap`: WILL NEVER BE IMPLEMENTED UNTIL FURTHER RESEARCHED.**
- Measured (32840 calls): 822 instructions per cached lookup. `var_value("PATH")` is 444 of them and the `strcmp` of
  PATH against the last-seen copy is 199, together 78%; the table itself (`exec_hashstr` twice, `exec_lookup`) is
  about 100. Swapping the table for `lib/hashmap` would therefore change almost nothing; step 2 above is the fix.
- Lines: `exec_hashstr.c`, `exec_lookup.c`, `exec_create.c` are about 60, `builtin_hash.c` and `exec_search.c` walk
  `exec_hashtbl` by hand in five places (`hashmap_next()` would shorten those). But the entry carries `hits`, `mask`,
  `cmd` and its own name, so the map would store a pointer to it: two allocations per command, as today, plus the
  key `strdup` that `hashmap_put2()` makes (and again on every rehash).
- Binary size: the `lib/hashmap` objects are 2958 bytes of text in the default build (a MinSizeRel figure is not
  measured) and are already linked when `awk` is; the shell would gain them only if built without `awk`. The saving
  from deleting `exec_hashstr/lookup/create` (about 60 lines) is far smaller than that, so size does not favour it.
- If it is ever tried, `lib/hashmap` should first learn: borrowed keys (`hashmap_put_borrowed`: no `strdup`, no free),
  `& (capacity - 1)` instead of `% capacity` per probe, a rehash that moves entries instead of re-`put`ting them, and
  an `hashmap_entry` embedded in the caller's struct (intrusive, no `void*` value). The first three also help `awk`.
  Open questions: does the shell link `awk` often enough to count the size as free; is a 32-slot chain really worse
  than open addressing at under 30 entries (probably not).
