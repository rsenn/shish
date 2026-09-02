# TODO / Roadmap

Leverage-sorted list of what's still open. Fixed work lives in `git log` and
`fixes/*.patch`, not here — this file only tracks what's left to do. See
`BUGS` for confirmed, reproducible defects with repro steps.

---

## MAIN QUEST — POSIX conformance and memory safety

Everything else in this file is secondary to this until it's done. The
site's own pitch (`docs/conformance.html`) already says shish is
"proof-of-concept quality" and targets POSIX "and nothing else" — the
job here is closing the gap between that claim and `tests/posix`'s
actual numbers, without introducing memory corruption while doing it.
Two stages, done in order, plus one requirement that runs continuously
underneath both:

1. **Stage 1 — the shell language itself.** Everything `tests/posix`
   measures that is not a builtin: signal disposition, which errors
   must exit the shell, expansion/quoting/parsing, control flow and
   exit status. This is Phases 1, 2, 4, 5 below (≈740 of the ≈822
   non-skip failures once the `sig*` family is fixed). Land this
   first — several builtin failures (e.g. `set -o` option names,
   `trap` printing) are thin wrappers over language-level state that
   Stage 1 fixes anyway.
2. **Stage 2 — utilities/builtins.** `test`, `alias`, `kill`, `read`,
   `command`, `unset`, `umask`, `set`, `shift`, `export` and the rest —
   Phase 3 below. Independent, per-builtin fixes; start once Stage 1's
   language-level failures stop shadowing them.
3. **Ongoing, throughout both stages — memory safety.** A frequent
   ASan+UBSan build is a gate, not a one-off cleanup pass: every change
   in Stage 1/2 must be re-verified under
   `-fsanitize=address,undefined` before being counted as done, the
   same way `tests/fixed.sh`/`ctest` already are. See "Memory safety"
   below for what is open there and how to run it.

**Non-goal, decided 2026-09-02:** bash's `var+=value` append-assignment
(not in POSIX; confirmed unimplemented -- `x=a; x+=b` parses `x+=b` as
a command name and fails). This is the reason libtool's `ltmain.sh`
can't run under shish, but libtool/libtool-generated scripts are not a
target -- don't add `+=` (as an opt-in flag, the same way `-B`/`-H`
gate brace/history expansion, or otherwise) unless that changes.

The measurable target for Stages 1-2 is `tests/posix` (yash's POSIX suite, 123 files).
Everything below is derived from full runs on 2026-08-20, at
`9bfd1f9f` plus `fixes/188`-`192`.

### Scoreboard (2026-08-21, after Phase 2 + `fixes/196`)

```
cases 12195   passed 5270   failed 822   skipped 6103
   failures:  sig*-p family 567  |  everything else 255
```

**Stale as of `fixes/213` (2026-09-02):** the `sig*-p` family is down
~500, to ≈67 — `sigint2-p`/`sighup2-p`/`sigquit2-p`/`sigterm2-p` are now
180/180 (were 124/180 each), `sigurg2-p`/`sigcont2-p` also 180/180. Full
scoreboard not yet re-run from a clean `ctest`; the per-file numbers
below (and the `sig*` breakdown after them) still reflect the pre-213
state except where a "Done, `fixes/213`" note says otherwise.

Where this came from: 3952 passed / 2140 failed before `fixes/190`-`192`
(Phase 1), 4863 / 1229 after, 5181 / 911 before Phase 2. Part of the
movement in those numbers is not a fix at all — it is measurement noise
in the signal files, see the warning below. The 6103 skips are not
passes — see Phase 6.

Per-file failure counts, everything except the `sig*` family:

```
48 alias-p  17/65    9 redir-p    52/61     4 shift-p   10/14   1 lineno-p   2/3
24 kill2-p   4/28     8 simple-p   26/34     3 return-p  22/25   1 function-p 18/19
22 read-p    6/28     8 set-p      37/45     3 input-p   8/11    1 fnmatch-p  6/7
18 quote-p  17/35     8 kill1-p     9/17     3 case-p    49/52   1 export-p   4/5
18 param-p  36/54     6 unset-p     6/12     2 dot-p     12/14   1 continue-p 30/31
16 test-p  220/236    6 exit-p      8/14     2 cmdsub-p  12/14   1 comment-p  14/15
15 command-p 34/49    4 tilde-p    25/29     1 builtins  80/81   1 break-p    31/32
 9 umask-p  74/83                            1 pipeline-p 8/9    1 async-p    8/9
 9 trap-p   28/37
```

The `sig*` family splits cleanly in two — the `*2-p`/`*6-p` files (the
"initially ignored" ones, 500 of the 567 failures) and everything else,
which is down to a handful of cases each:

```
59 sighup6  121/180   56 sigint2  124/180   16 sigurg2  164/180    3 sigurg6  177/180
56 sigterm2 124/180   55 sigterm6 125/180   16 sigquit5 164/180    3 sigurg5  177/180
56 sigquit2 124/180   54 sigquit6 126/180   16 sigint5  164/180    3 sigurg1  177/180
56 sighup2  124/180   54 sigint6  126/180   16 sigcont2 164/180    3 sigterm1 177/180
                                            11 sigquit1 169/180    3 sighup5  177/180
                                            11 sigint1  169/180    3 sighup1  177/180
                                             8 sigterm5 172/180    3 sigcont6 177/180
                                                                   3 sigcont5 177/180
                                                                   3 sigcont1 177/180
```

**Do not trust a signal-file number from a busy machine.** The same
binary scored `sigterm1-p` 36/180 in one `ctest` run and 177/180 in the
next, and `sigterm6-p` 89 then 125 — the difference tracks what else
was running at the time, not the code. Measure the `sig*` files on an
otherwise idle machine, and re-measure before concluding anything from
a change in them (`BUGS: signal-tests-vary-with-machine-load`).

Clean: `andor arith cd errexit eval exec for fsplit getopts grouping
if kill4 nop option path ppid readonly until while`. `kill3-p` is
neither — it times out (`BUGS: kill-stop-self-in-subshell-deadlock`).

### How to measure

```sh
# one file (testee path MUST be absolute)
sh tests/run-tst.sh "$PWD/build/x86_64-linux-gnu/shish" tests/posix exec-p.tst

# whole suite
(cd build/x86_64-linux-gnu && ctest)

# scoreboard from the .trs files the run leaves behind
cd tests/posix && for f in *.trs; do
  t=$(grep -Ec '^%%+ (PASSED|FAILED|SKIPPED):' "$f")
  x=$(grep -Ec '^%%+ FAILED:' "$f"); s=$(grep -Ec '^%%+ SKIPPED:' "$f")
  [ "$x" -gt 0 ] && printf '%4d %-14s %d/%d\n' "$x" "${f%.trs}" "$((t-x))" "$t"
done | sort -rn

# what a family is actually failing on
grep -h -E '^%%+ FAILED' tests/posix/sig*.trs | sed -E 's/.*: SIG[A-Z]+ //; s/ \(.*//' \
  | sort | uniq -c | sort -rn
```

Every phase below ends the same way: rerun the named files, record the
new count here, remove the closed `BUGS` entry, add `fixes/NN` + a case
in `tests/fixed.sh`.

---

### Phase 1 [Stage 1: language] — signal disposition (≈67 left, was 1790)

Done, `fixes/190`-`192`: `trap '' SIG` now ignores instead of
resetting to the default; `trap - SIG` for an untrapped signal is a
no-op success instead of a special-builtin error that exits the shell;
SIGHUP is trappable (`if(signum != 1)` skipped it); a trap fires
between `;`-separated commands, not only at line boundaries; a trap
body may uninstall its own trap without freeing the tree it runs from;
and a signal-killed child sets `$?` to 128+N instead of 0.

Done, `fixes/213`: **a signal ignored on entry to a non-interactive
shell cannot be trapped or reset** (POSIX 2.11) — this was the whole
story for the four `*2-p` files (`sigint2-p`/`sighup2-p`/`sigquit2-p`/
`sigterm2-p`, 124/180 each) plus part of `sigurg2-p`/`sigcont2-p`.
`sh_init()` now snapshots each signal's disposition once
(`sig_snapshot()`/`sig_was_ignored()`, `lib/sig/sig_snapshot.c`) before
anything else touches one, and `trap_install()`/`trap_uninstall()`
no-op for a signal that was already `SIG_IGN`, gated on
`!(source->mode & SOURCE_IACTIVE)` — an interactive shell has no such
restriction, confirmed against `tests/posix/signal.sh`'s own
`final_trap=ignore` rule. A second, independent bug was hiding behind
this one and had to be fixed in the same pass: `sh_main.c`'s `-i`/`+i`
startup-option parsing ignored the `+`/`-` prefix entirely, so both
forced interactive regardless of which was given — meaning `*2-p`
(`+i`, non-interactive) and `*6-p` (`-i`, interactive) were both
actually running interactively, which is why fixing only the trap
logic first sent `*6-p` backwards (126→68/180) until this was also
fixed. All four `*2-p` files, plus `sigurg2-p`/`sigcont2-p`, are now
180/180; `*6-p`/`*1-p`/`*5-p` (the genuinely-interactive and
default-disposition combos) are unchanged, confirmed via a full
`ctest` diff against a stashed-back baseline.

What is left, in order:

1. **Disposition across fork and exec.** POSIX: a signal the shell
   traps to a command is reset to the default in a forked/exec'd
   child (an ignored one already stays ignored for free now, since
   `fixes/213` never touches its real disposition at all).
   `job_fork.c`/`exec_program.c` do not do this systematically yet.
2. Re-triage the remainder — every `sig*-p` file is now at 164-180/180,
   so what is left is a handful of individual cases per file, not a
   family-wide cause. Re-run the scoreboard commands below first;
   the per-file counts above this section predate `fixes/213`.

### Phase 2 [Stage 1: language] — error semantics: which failures must exit the shell (0)

Done, `fixes/196`: `error-p` is 212/212, from 127/212. POSIX 2.8.1 says
exactly which errors kill a non-interactive shell; the decision now
lives in one place, at the end of `eval_simple_command()`:

- an expansion error (`${x?msg}`, `$x` under `set -u`) exits a
  non-interactive shell and fails only the command in an interactive one
- an assignment error (`readonly r=1; r=2`) exits, for any command
- a redirection error exits only for a special builtin or `exec`; on a
  plain utility the command does not run and the shell carries on
- a syntax error in the string `eval` was given is reported and fails
  (`eval` is a special builtin, so that exits too) — it used to print
  nothing at all
- `shift` was `B_DEFAULT`, so none of the special-builtin rules applied
  to it. It is `B_SPECIAL` now, which also means `shift 5` with `$# = 2`
  ends a non-interactive shell, as in dash

Two fd bugs had to be fixed to get there: `source_flush()` dropped the
whole read-ahead buffer after a syntax error (swallowing every later
command when input came from a file, not a terminal), and `exec <file`
destroyed `fdtable[n]` — closing its descriptor — *before* opening the
file, so `exec <_no_such_file_` left an interactive shell with no stdin.
The file is opened first now (`redir_preopen()`) and handed over
(`fdtable_openfd()`).

What is left of this subject is diagnostics only: shish prints
`file:LINE:COL: msg` where the line number is one too high (the parser
has already advanced) and omits the offending name. `echo ${x?boom}` on
line 2 reports `:3:1: boom`; bash reports `line 2: x: boom`. `$LINENO`
itself is correct. Fix with, and verify against, `lineno-p.tst` (2/3)
and `BUGS: error-message-line-number-off-by-one`.

---

### Phase 3 [Stage 2: builtins/utilities] (≈100 failures, each step independent)

Sorted by failures per unit of work.

1. **`test`/`[` segfaults on `test 1 -a 1`** (also `-o`, also with
   empty operands; `test -n x -a -n y` is fine).
   `BUGS: test-binary-and-or-segfault`. The fix is POSIX's own
   argument-count algorithm
   (1/2/3/4-argument forms decided by count first, operator second)
   rather than dispatching on operator position.
   → `test-p.tst` 220/236, plus `test ! = !`, `test ( = )`.
2. **`alias` (17/65).** Bare `alias` prints nothing at all; `alias
   name` prints correctly. Printing must be reusable as input, and
   subshells must inherit aliases. `BUGS:
   alias-printing-and-substitution-broken`.
3. **`kill` (24 + 8).** `kill -s NAME` fails for most names
   (`ABRT ALRM BUS FPE HUP ILL KILL QUIT ...`), though `kill -l`
   prints them; and `kill -s HUP 0` does not work. Likely one
   name→number lookup path.
4. **`read` (22).** IFS whitespace vs non-whitespace splitting,
   backslash continuation, reading no more than one line.
   `BUGS: read-field-splitting-and-options-broken`.
5. **`command` (15)** — `-v`/`-V` output formats for builtins,
   externals with and without a slash; and a not-found dot script
   must not kill the shell.
6. **`unset` (6)** — `-f` (functions) does not delete; readonly
   variables must not be deletable.
7. **`umask` (9)**, **`set` (8)**, **`shift` (4)**, **`export` (1)**.
   `set -o` is missing the POSIX names `ignoreeof`, `nolog`,
   `notify`, `verbose`, `vi` (it lists the bash extras
   `braceexpand`/`hashall`/`histexpand`/`privileged` instead) —
   folds in `BUGS: set-notify-unimplemented`,
   `set-verbose-unimplemented`, `set-histexpand-unimplemented`.

---

### Phase 4 [Stage 1: language] — expansion and parsing (≈60)

1. `quote-p` (17/35) — backslash and line continuation inside
   reserved words, operators, parameter expansions.
   `BUGS: quote-backslash-escaping-broken`.
2. `param-p` (36/54) — assignment to readonly/positional/special
   parameters, `${#...}` edge cases, pattern removal edge cases (the
   basic forms all work, so this entry is narrower than
   `BUGS: param-expansion-pattern-removal-broken` claims).
3. `redir-p` (52/61) — tilde expansion in redirection operands,
   heredocs on a non-default fd, several heredocs per command, long
   heredocs. `BUGS: redir-tilde-expansion-and-heredoc-broken`.
4. `simple-p` (26/34) — redirections must precede assignments for a
   non-special builtin; PATH search rules; command name with a slash.
5. `tilde-p` (4), `case-p` (3), `fnmatch-p` (1), `cmdsub-p` (2),
   `comment-p` (1) — small, individually filed in `BUGS`.

---

### Phase 5 [Stage 1: language] — control flow and exit status (≈15)

1. `exit-p` (8/14) — default exit status in a subshell and inside a
   trap; `exit N` from a trap. `BUGS:
   exit-status-in-trap-and-subshell-broken`.
2. `trap-p` (28/37) — trap printing (`trap`, `trap -p`), numeric
   signal operands, and what a trap sees of the redirections in
   effect when it was set.
3. `return-p` (22/25, was 0/25 before the debug prints came out) —
   only "default exit status of returning from function/dot script"
   is left. `BUGS: return-default-exit-status-wrong`.
4. `break`/`continue` inside `eval` still no-ops
   (`BUGS: break-continue-inside-eval-no-op`) — `eval`'s frame reuses
   `E_ROOT` for an unrelated purpose; give it its own flag.
5. `input-p` (3) — the shell reads ahead past the current line.
6. `dot-p` (2), `function-p` (1), `pipeline-p` (1), `async-p` (1),
   `break-p` (1), `continue-p` (1), `export-p` (1).

---

### Phase 6 [Stage 1+2] — what is not being measured at all

1. ~~**6103 of 12195 cases are skipped**~~ — **done, 2026-09-02**: the
   44 `%REQUIRETTY%` files (the `sigttin`/`sigttou`/`sigtstp`/`sigstop`
   `*3-p`/`*7-p`/`*8-p` combos, `kill4-p`, `bg-p`/`fg-p`/`job-p`,
   `testtty-p`, `wait-p`) skip themselves via `../checkfg`
   (`tests/checkfg.c`, restored from git history — it had gone missing
   from the tree, silently turning every one of these into an
   unconditional "command not found" skip rather than a real check) —
   they don't need a pty simulator so much as a real one.
   `tests/pty-run.c` (a single-file POSIX-`pty` wrapper: `posix_openpt`/
   `TIOCSCTTY`, no libc convenience `forkpty()`) gives the testee a
   genuine controlling terminal and session; `-DDO_PTY_TESTS=ON` wires
   it into `CMakeLists.txt` for every `%REQUIRETTY%` file. First real
   run: **10/44 pass** (`testtty-p` and the `kill`-driven `*4-p`
   combos), 34 fail — some are ordinary conformance gaps (e.g.
   `sigcont3-p`'s 3 real output mismatches, same shape as everything
   else in this file), but most of the `*3-p`/`*7-p`/`*8-p` combos
   plus `wait-p`/`kill4-p` hang the full 60s `pty-run` alarm instead of
   completing — a real job-control defect, since their `kill`-driven
   `*4-p` siblings pass in 1-2s. One case is already traced:
   `BUGS: wait-interrupted-by-trap-hangs` (`wait` doesn't get
   interrupted by an arriving trapped signal). The rest need the same
   per-case bisection — `BUGS:
   job-control-real-terminal-hangs-vs-kill-driven-ok`. Re-run via:
   ```sh
   cmake -S . -B build/x86_64-linux-gnu -DDO_PTY_TESTS=ON
   cmake --build build/x86_64-linux-gnu -j
   cd build/x86_64-linux-gnu
   NAMES=$(grep -l '%REQUIRETTY%' ../../tests/posix/*.tst \
           | xargs -n1 basename | sed 's/\.tst$//' | tr '\n' '|' | sed 's/|$//')
   ctest -R "posix/(${NAMES})\.tst\$" -j4
   ```
2. **`tests/yash` (119 files) is off by default** — several files
   (`arith-y`, `cmdprint-y`, `pipeline-y`, `redir-y`, `until-y`,
   `while-y`) hang, none isolated. `BUGS: yash-suite-other-hangs`.
   Isolate one hang per session; each is likely its own bug.
3. `grouping-p.tst:34` is flaky (~1 run in 5) — a real race between a
   subshell's background writer and the FIFO read after it.
   `BUGS: grouping-p-tst-flaky`.
4. The harness leaves `tests/posix/tmp.NNNNN/` behind on every hard
   failure (97 accumulated). Clean them up and make the harness
   remove its own.
5. `tests/fixed.sh` fails 3 of its own assertions on a default build:
   they assume the optional `cat`/`rm` builtins are compiled in.
   `BUGS: fixed-sh-assumes-optional-builtins`.

---

### `BUGS` ↔ conformance-gap map

`BUGS` currently holds 34 entries. Sorted by whether they explain part
of the `tests/posix` gap above, or are unrelated maintenance items:

**Directly explains a scoreboard number (fix these as part of Stages 1-2):**

- `posix-signal-ignored-on-entry-can-be-trapped`, `signal-tests-vary-with-machine-load`,
  `kill-stop-self-in-subshell-deadlock` → Phase 1 (`sig*-p`, `kill3-p`).
- `error-message-line-number-off-by-one` → Phase 2 (`lineno-p`).
- `alias-printing-and-substitution-broken`, `read-field-splitting-and-options-broken`,
  `set-notify-unimplemented`, `set-verbose-unimplemented`,
  `set-histexpand-unimplemented` → Phase 3 (`alias-p`, `read-p`, `set-p`).
- `quote-backslash-escaping-broken`, `param-expansion-pattern-removal-broken`,
  `redir-tilde-expansion-and-heredoc-broken`, `case-pattern-expansion-broken`,
  `case-pattern-bracket-quote-stripping`, `fnmatch-quotation-of-quotations` →
  Phase 4 (`quote-p`, `param-p`, `redir-p`, `case-p`, `fnmatch-p`).
- `exit-status-in-trap-and-subshell-broken`, `return-default-exit-status-wrong`,
  `break-continue-inside-eval-no-op`, `input-not-read-line-wise` → Phase 5
  (`exit-p`, `return-p`, `break-p`/`continue-p`, `input-p`).
- `yash-suite-other-hangs`, `grouping-p-tst-flaky`, `fixed-sh-assumes-optional-builtins` →
  Phase 6 (what the scoreboard doesn't even measure yet).

**Real bugs, but not counted in the `tests/posix` scoreboard at all** (fix
opportunistically, not blocked on Stage 1/2, don't expect the score to move):

- `eval-lineno-imprecise-inside-function`, `nested-heredoc-in-cmdsub-hangs`,
  `exit-trap-loses-positional-parameters`,
  `help-builtin-columns-overlap`, `no-tree-print-option-is-a-noop`,
  `cfg-cmake-mingw-silently-builds-native`, `eval-node-bgnd-silent-on-fork-failure`.

**Memory safety, not conformance** — tracked under "Memory safety" below
instead: `asan-leak-residue-not-fully-triaged`,
`ubsan-buffer-op-proto-function-type-mismatch`,
`debug-build-asserts-on-source-after-external-cmdsubst`,
`builtin-fork-races-sh-onsig-sigchld`.

### Memory safety [ongoing, both stages] — ASan+UBSan as a recurring gate

Not a phase with an end state — a build that has to be run frequently
(every fix in Stages 1-2, not just periodically) so a language/builtin
fix doesn't trade a conformance failure for a corruption bug:

```sh
cmake -B build/asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan
(cd build/asan && ctest)         # ASan/UBSan abort = immediate hard failure
```

What's currently open under this build, from `BUGS`:

1. `asan-leak-residue-not-fully-triaged` — most leaks fixed
   (`fixes/129`, `130`, `133`); three call sites still untriaged
   (`redir_parse.c:108`, `expand_args.c:67`, `eval_function.c:57`) plus
   one root-caused-but-unfixed 36-byte leak per backgrounded command
   (`tree_cat()` building `job->command`, `eval_simple_command.c:261`).
   `struct var` and `sh_loop()`'s scratch stralloc's are permanent,
   process-lifetime state and are not bugs.
2. `ubsan-buffer-op-proto-function-type-mismatch` — `lib/buffer.h`'s
   `buffer_op_proto` cast onto libc `read`/`write` is UB by the letter
   of the standard but not fixable without wrapping two libc functions
   everywhere for no observable effect; not planned to change. The two
   real mismatches in unused `lib/buffer/` glob-compiled dead code are
   also left alone per this repo's "don't touch unused `lib/` code
   unasked" standard.
3. `debug-build-asserts-on-source-after-external-cmdsubst` — an
   assertion firing only under `BUILD_DEBUG`/ASan builds, not release;
   needs triage before Stage 1 work can trust debug-build test runs.
4. `builtin-fork-races-sh-onsig-sigchld` — a real fork/signal race
   (not just a sanitizer artifact); most likely to surface as a
   flaky, hard-to-reproduce ASan failure during Stage 1/2 work rather
   than its own dedicated session, so flag it here rather than let it
   get blamed on whatever fix was running at the time.
5. **Goal 4** below (fd/fdtable/redir vs. non-forking subshells) is
   also a memory-safety item, not just a conformance one — its "Still
   open" problem 3 has a confirmed heap-corruption repro. See that
   section for the full writeup; it's kept separate because of its
   length, not its priority.

---

## Everything below this line is lower priority than the MAIN QUEST above.

---

## Goal 2 (secondary) — `src/job` cleanup

Small, isolated, low-risk items left over from a full audit of
`lib/sig`/`lib/wait`/`src/job` (most of that audit's findings are already
fixed — see `fixes/41` through `fixes/76`):

- **Delete `src/job/job_x.c`.** Byte-for-byte duplicate of
  `job_printstatus.c`, confirmed zero callers anywhere.
- **Delete `job_get`/`job_proc`/`proc_bypid`** (`src/job.h`,
  `src/job/job_get.c`) — confirmed zero callers anywhere.

---

## Goal 3 (secondary) — arena allocator for the AST (planned, not started)

`src/tree.h`'s AST is a graph of individually `malloc()`'d nodes
(`tree_newnode()`) plus separately `malloc()`'d string buffers hanging off
several of them — one `malloc`/`free` pair per node, even though a tree's
real lifetime is always "parse it all at once, evaluate, throw the whole
thing away" (`sh_loop.c`). `lib/arena.h` already declares the target
interface (not implemented yet):

```c
typedef struct arena_s { struct arena_block* head; size_t blocksize; } arena;

void  arena_init(arena* a, size_t blocksize);
void* arena_alloc(arena* a, size_t len);  /* bump-allocate, no per-call free */
void  arena_reset(arena* a);              /* forget contents, keep blocks for reuse */
void  arena_free(arena* a);               /* release everything back to the system */
```

Design decisions already worked out (full reasoning in git history —
2026-07-23/24 commits):

- **A stack of arenas, not one global one.** Every independent
  parse-evaluate-free scope (`sh_loop.c`, `builtin_eval.c`,
  `builtin_source.c`, `builtin_expr.c`, `prompt_parse.c`,
  `builtin_trap.c`'s inline parse) nests strictly via ordinary call-stack
  recursion — shish is single-threaded, so arenas never need to overlap
  without nesting. Push one per scope; `arena_reset()`/`arena_free()` it
  wherever `tree_free()` is called today.
- **`tree_free()` mostly disappears, not just changes signature.** Most of
  its current call sites just free a subtree still inside the current
  statement — those calls simply go away, since the dead nodes just wait
  for the enclosing arena to reset. Only the handful of true scope
  boundaries above get an `arena_reset()`/`arena_free()` call instead.
- **Two things can't live in the transient arena:** function bodies and
  trap bodies, since both must outlive the statement that defines them.
  Trap bodies already parse through their own independent `parse_init()`
  call, so they can just get their own dedicated, never-reset arena.
  Function bodies parse inline as part of the defining statement;
  `eval_function.c` used to keep them alive past their own tree with a
  manual "steal the body pointer, null the original" trick, but that
  broke (segfault) the moment the same definition node was evaluated
  more than once — a function defined inside a loop, or inside one of
  shish's in-process `(...)` subshells — since the second visit found
  the pointers already nulled from the first (fixed 2026-07-30,
  `fixes/102`). It now does the "deep-copy into long-lived storage at
  adoption time" option this bullet already anticipated, via a new
  generic `tree_copy()` (`src/tree/tree_copy.c`) that mirrors
  `tree_free()`'s per-kind switch. Once the arena lands, `tree_copy()`
  is exactly the function that needs to switch from allocating loose
  nodes to bump-allocating into a function's own dedicated arena instead
  — the "parser switching allocators while inside a function body"
  alternative is no longer needed now that a working copy path exists.
- **`stralloc` doesn't fit an arena** — it grows via `realloc()`, which
  can't work once other data has been bump-allocated after it. A new,
  immutable type covers the tree's own write-once-at-parse-time strings:

  ```c
  typedef struct { size_t len; char* s; } arena_str; /* NUL-terminated,
                                                         built once */
  arena_str arena_strcpy(arena* a, const char* s, size_t len);
  ```

  This replaces `nargstr.stra`, `nargparam.name`, `nfor.varn`, and
  `nfunc.name` (all populated once, during parsing, never touched again).
  `narg.stra` stays a real `stralloc` — it's populated later, at
  expansion time, not parse time. Packing a node and its string tightly
  adjacent in the arena is safe with no alignment padding, since
  `src/tree.h`'s node structs are already `__packed`.
- **Possible future: precompiled/cached AST on disk.** Serialize arena
  blocks with node pointers rewritten to offsets; on load, run one linear
  fixup pass turning offsets back into real pointers (structured like
  `tree_free()`'s own `switch(node->id)`) — after that, every existing
  tree-walking function works completely unmodified. A more invasive
  "offsets natively everywhere, zero-copy `mmap()`" design is possible but
  touches every tree-walking call site for a benefit unlikely to matter
  next to lexing/parsing cost.

---

## Goal 4 (secondary, but overlaps MAIN QUEST memory safety) — `fd`/`fdtable`/`fdstack`/`redir`: the fd≤2 protection is load-bearing, not incidental

Grew out of chasing `exec >&2 2>/dev/null; echo reached` sending
"reached" to the wrong stream (fixed 2026-08-21, `fixes/193`). Two
fix attempts (2026-08-17) each got the original bug's repro passing, and
each broke something bigger — a segfault in one case, all external-command
pipelines in the other. Both were reverted at the time; the sections
below (in original, chronological order — read "Current status" first,
it's the fast-start summary) are the full investigation writeup, plus
(see "Progress", both passes) the refactorings that were since
implemented and verified regression-clean.

### Current status (2026-08-18) — read this first

**Fixed and merged, regression-tested, zero known regressions:**

- `fdtable_gap()`'s unsafe eviction branch (`src/fdtable/fdtable_gap.c`)
  — was destroying a merely-shadowed struct's real fd outright; now
  relocates it via a fresh `dup()` instead. fixes/186 (2026-08-18
  pass) supersedes the narrower "fixed 2026-08-17" version referenced
  later in this file — the 2026-08-17 fix (unconditional
  `fd_setfd(gap,-1)` before `fd_pop`) was real but only *necessary*,
  not sufficient; the relocate-instead-of-destroy behavior for the
  `FDTABLE_FORCE && gap != fdtable[gap->n]` branch specifically is
  what the 2026-08-18 pass added on top.
- `fdtable_dup()`'s `dup2()`-landing branch (`src/fdtable/fdtable_dup.c`)
  — same bug, second instance, fixed the same way (relocate before the
  `dup2()` call, since `dup2()` can't be undone after the fact).
- `fd_close()` (`src/fd/fd_close.c`) — now checks `fd_list[]` against
  `rb.fd`/`wb.fd` directly (not gated on `fd->e` or `FD_DUP` mode)
  before actually calling `close()`, so a struct that's lost real
  ownership of its number gets neutered instead of double-closing
  someone else's fd.
- `lib/buffer/buffer_close.c`'s `if(b->fd > 2)` guard is gone —
  narrowed to plain `if(b->fd >= 0)`, restoring it to ordinary
  upstream `libowfat` behavior. This was only safe to do *after* the
  three fixes above; don't re-add the `> 2` guard as a quick fix for
  anything, it was a symptom-masking workaround, not a feature.
- The subshell segfault (`( exec 3>&1 1>&2 2>&3 3>&- ; ... )` pattern)
  — fixed via `struct fd_state`/`fd_state_save()`/`fd_state_restore()`
  (`src/fd.h`, `src/fd/fd_state_save.c`, `src/fd/fd_state_restore.c`),
  called around `eval_subshell()`'s existing `fdstack_push()`/
  `fdstack_pop()` pair.
- Patches: `fixes/186-fd-table-bookkeeping-vs-real-close-desync.patch`,
  `fixes/187-subshell-fd-table-not-scoped.patch`. Regression tests:
  `tests/fixed.sh`, search for `fixes/186`/`fixes/187`.
- Verification performed: both original repros (the pipeline
  corruption and the subshell segfault) clean 10/10 runs each; full
  `tests/fixed.sh` and full `ctest` (incl. `tests/posix/*.tst`)
  produce byte-identical pass/fail lists against a stashed-back
  baseline (`git stash push -- <the changed files>`, rebuild, rerun,
  diff the sorted failure lists — that's the exact technique used and
  the one to repeat for any future change here).

**Still open — this is where a fresh pass should start:**

1. ~~**The original `exec >&2 2>/dev/null` symptom**~~ — fixed
   2026-08-21 (`fixes/193`), on the third attempt, exactly as this
   entry predicted: `redir_dup()` resolves a persistent dup eagerly
   via `fdtable_dup(nredir->fd, FDTABLE_FORCE | FDTABLE_CLOSE)` right
   after `fd_dup()`. The two earlier attempts failed on bugs that
   `fixes/186`/`fixes/187` had since fixed; what was still needed on
   top was one guard — **do it only outside a subshell**
   (`!exec_subshell_depth`), see problem 3 below. `exec-p.tst` 9/10 ->
   10/10, full `ctest` and `tests/fixed.sh` otherwise unchanged.

2. ~~**No general fdstack-scoped ownership tracking**~~ — narrowed and
   closed for every call site that actually has the exposure
   (2026-08-21, `fixes/194`). The scoping unit is not the fdstack
   level: a brace group hosting an `exec` (`{ exec 4>&3; } 3>&2`) must
   keep its effects when its own level pops, so saving/restoring the
   globals per level would be wrong. The unit is the *subshell
   environment* — an in-process scope that does `sh_push()`. There are
   exactly two: `eval_subshell()`, which already had
   `fd_state_save()`/`fd_state_restore()`, and `expand_command()`
   (`$(...)`, a subshell per POSIX 2.6.3), which had neither that nor
   the trap snapshot. The missing trap snapshot was a live bug, not a
   risk: `X=$(trap 'echo T' TERM; echo x)` left that trap installed in
   the calling shell. Both are covered now; what remains is a
   convention, not a gap — a new in-process subshell scope has to
   repeat the same six saves (fdstack, fd_state, vartab, env,
   functions, traps).

3. **Persistent (`exec`) redirections vs. the non-forking subshell
   model are still fundamentally in tension** (problem 3 below,
   unstruck). `eval_subshell()` still runs `(...)` in-process, and
   `fd_new()`/`fdtable_newfd()` still can't distinguish "persistent
   for the rest of the process" (true at the real top level) from
   "persistent only for this subshell's lifetime" (true inside a
   non-forking subshell). The `fd_state_save`/`restore` fix papers
   over the *bookkeeping* half of this (confirmed sufficient to kill
   the segfault), but does nothing about real `dup2()`/`close()`
   syscalls a persistent redirection inside a subshell already issued
   against a still-live descriptor before the subshell returns — see
   "Suggested refactorings" below for the two concrete options (make
   subshells fork when they contain a persistent redirection, or teach
   the fd-table to scope "persistent" to the enclosing fdstack level).
   **Attempted 2026-08-21, not fixed — read this before trying
   again.** There is now a concrete demonstration: drop the
   `!exec_subshell_depth` guard `fixes/193` added to `redir_dup()`,
   and

   ```sh
   ( exec 3>&1 1>&2 2>&3 3>&- ; echo hi ) >/dev/null 2>&1
   /bin/true
   ```

   prints `fdtable: redirection cycle detected` from the forked child
   at `fdtable_exec()` time, and segfaults outright deeper into a
   longer script (`tests/fixed.sh` dies at its own `fixes/73` swap
   case). At the point of the error `fdtable[1]` has `n=1, e=4` while
   the real fd 1 is held by another struct that also wants slot 1, so
   `fdtable_gap()` recurses into resolving it and trips the cycle
   check.

   The obvious reading of that — "the subshell's real `dup2()`s
   outlived it while `fd_state_restore()` put the bookkeeping back" —
   is **not the whole story, and fixing only that is not enough.**
   Tried and rejected: a per-scope pre-image list (`fd_scope_park()`
   called just before `fdtable_dup()`'s `dup2(o, d->n)`, parking the
   old descriptor with `fcntl(F_DUPFD, 64)`, restored with the owning
   `fdtable[]` entry on scope exit). It does fire where you would
   expect — 4 parks for the swap repro above — and it is harmless with
   the guard in place, but with the guard removed the cycle error is
   *unchanged* and heap corruption follows. So the leftover state that
   trips `fdtable_gap()` is in the fd table's own entries and
   `fd_list[]`, not merely in the kernel descriptors. One more note:
   an early attempt that parked *every* open fd
   at scope entry (rather than lazily, per overwritten number)
   deadlocked the suite — an extra copy of a pipe's write end keeps
   its reader from ever seeing EOF. Do it with the fd table in front
   of you this time (`fixes/195` made that possible again):

   ```sh
   cmake -S . -B build/dbg -DCMAKE_BUILD_TYPE=Debug \
         -DDEBUG_FDTABLE=ON -DDEBUG_FD=ON -DDEBUG_FDSTACK=ON
   build/dbg/shish -c 'exec 3>&1; dump -t'   # -t table, -s stack, -f list
   ```

The rest of this section (below) is the original, in-order investigation
writeup — root cause, call-site inventory, the full struct-lifecycle
trace for the subshell crash, and the two "Progress" logs (2026-08-17,
2026-08-18) each pass was written up in at the time. Keep reading if
you need the *why*, not just the *what*.

### Where the guard actually came from, and what leans on it

`lib/buffer/buffer_close()` is a plain `libowfat` primitive, shared by
code that has nothing to do with shish's fd table (`lib/path/path_gethome.c`,
`lib/stralloc/mmap_filename.c`). Its `if(b->fd > 2) close(b->fd);` guard
is upstream `libowfat`, not shish-specific — but `src/fd/fd_close.c`
(shish's only fd-table-aware caller) was written *assuming* that guard is
there. `fd_close()` is called every time any `struct fd` is torn down —
every temporary redirection's fdstack level pop, every persistent
(`exec`) redirection's `fdtable_newfd()`-driven reinit, all of it — and it
unconditionally calls `buffer_close(&fd->rb)`/`buffer_close(&fd->wb)`. As
long as `buffer_close()` refuses to actually `close()` fd 0/1/2, every one
of those teardown call sites gets a free pass: it doesn't matter whether
the particular `struct fd` genuinely owns its low-numbered real
descriptor or is just shadowing/aliasing it, the guard makes the mistake
harmless. Nothing in `src/fd/`, `src/fdtable/`, or `src/fdstack/` states
this invariant anywhere — it's implicit, and it is why `exec`'s own
persistent redirections of fd 0/1/2 can never really free/reclaim them
for reuse (the actual mechanism behind the original bug this was chasing).

### What happens when the guard is removed

Removing it (`if(b->fd >= 0) close(b->fd);`) does not break anything by
itself in isolation — `echo hi | sed 1q` and `exec >&2 2>/dev/null; echo
reached` both still worked run standalone. It only broke once exercised
inside the full `tests/fixed.sh` run (hundreds of commands deep), where
external-command pipelines inside command substitutions started failing
with `Bad file descriptor` on the child's stdin (`sed`, `grep`, `tr` all
affected). That it doesn't reproduce from a 5-line isolated repro, only
after dozens of preceding tests, is itself informative: this is bookkeeping
*drift*, not an immediate crash — some earlier command leaves
`fd_expected`/`fd_list[]` quietly inconsistent with reality, and a much
later, syntactically unrelated command is the one that visibly trips over
it (its `pipe()`/`open()` call lands on a real fd number that's secretly
still in use, and closing/using that fd is what fails).

The concrete unsafe path: `fdtable_gap()` (`src/fdtable/fdtable_gap.c`)
has two branches for evicting whatever currently sits in the way of a
`FDTABLE_FORCE`d fd:

```c
if(flags & FDTABLE_NOCLOSE) {
  fd_setfd(gap, -1);   /* neuter gap->rb.fd/wb.fd to -1 first */
  fd_pop(gap);          /* now safe regardless of what fd_close() does */
  return e;
}
fd_pop(gap);             /* <-- no fd_setfd(gap, -1) first! */
return FDTABLE_DONE;
```

The first branch is careful — `fd_setfd(gap, -1)` deliberately runs before
`fd_pop()`/`fd_close()`/`buffer_close()`, so whatever `close()` policy
`buffer_close()` has doesn't matter; `gap` never really owned the number
by the time it's torn down. The second branch has no such guard: it
`fd_pop()`s `gap` — real `struct fd`, real `rb.fd`/`wb.fd` still pointing
at the live low-numbered descriptor — directly. With the old
`buffer_close()`, that was harmless (the close silently no-op'd for
fd ≤ 2). With the guard removed, it's a real `close()` of a descriptor
that something else — a shadowed struct one fdstack level down, a pipe
endpoint about to be `dup2()`'d in a forked child, `fd_list[]`'s own
bookkeeping for that number — still expected to be alive.

`FDTABLE_NOCLOSE` is only added automatically by `fdtable_dup()`'s own
call into `fdtable_wish()`. Every *other* path into `fdtable_gap()` goes
through `fdtable_wish()` via `fdtable_open()` or `fdtable_here()`,
carrying whatever flags *their* caller happened to pass — and several
callers pass `FDTABLE_MOVE`/`FDTABLE_FORCE` alone, no `NOCLOSE`:

- `src/redir/redir_open.c:28` — `fdtable_open(nredir->fd, FDTABLE_MOVE)`,
  called directly (not through `fdtable_resolve()`) for every `R_NOW`
  redirection, which includes ordinary *temporary* redirections on a
  command with no argv at all (`READONLYVAR=changed 2>/dev/null` —
  `args == NULL` forces `R_NOW` in `eval_simple_command.c`, unrelated to
  `exec`). This is the exact command sitting immediately before the first
  pipeline that failed in the `tests/fixed.sh` run.
- `src/fdtable/fdtable_exec.c:41` — `fdtable_resolve(fdtable[i],
  FDTABLE_FORCE)`, run over *every* live fd right before every `execve()`
  (so also for a plain, redirection-free `sed`/`grep`/`tr` in a pipeline).

Neither of those is `exec`-specific or unusual — they're on the hot path
for practically every external command shish runs. That's why removing
the guard has such a broad blast radius: the unsafe `fdtable_gap()` branch
was always reachable during perfectly ordinary command execution, it just
never had a live low fd to damage before, because `buffer_close()` was
quietly eating every attempt to actually close one.

### The stdout/stderr-swap subshell crash: a related but distinct failure

The other fix attempt (forcing `redir_dup()` to eagerly resolve a
persistent dup via `fdtable_dup(nredir->fd, FDTABLE_FORCE | FDTABLE_CLOSE)`,
*without* touching `buffer_close()` at all) crashed reliably — but only
from inside a subshell, and only in code that runs *after* the subshell
returns:

```sh
X=$(mktemp); Y=$(mktemp)
( exec 3>&1 1>&2 2>&3 3>&-; echo a; echo b >&2 ) > "$X" 2> "$Y"
echo "exit=$?"; cat "$X"; cat "$Y"
```

Struct-`fd` lifecycle for this, traced end to end:

1. `eval_command()` sees the `( ... ) > "$X" 2> "$Y"` compound command has
   redirections → `fdstack_push()`s a new level **L1**. For each of
   `> "$X"`/`2> "$Y"`: `d = fd_alloc()` (a *temporary*, C-stack-allocated
   struct) → `fd_push(d, ...)` links it at L1, shadowing whatever was at
   the slot before (the shell's real, permanent, level-**L0** fd 1/2
   structs — the ones wrapping the process's actual inherited stdout/
   stderr). `redir_open()` really `open()`s `$X`/`$Y`, landing on some
   real fd (say 5 and 6) via `fd_setfd()`.
2. `eval_subshell()` runs. Because shish subshells never `fork()`
   (`eval_subshell.c` runs the body in-process via `fdstack_push()`/
   `setjmp()`), everything from here on shares the *same* process, the
   *same* global `fdtable[]`/`fd_list[]`/`fd_expected`. It pushes another
   level, **L2**.
3. `exec 3>&1 1>&2 2>&3 3>&-` runs. `exec`'s redirections are always
   *persistent* (`d == NULL`), so each goes through `fd_new()` →
   `fdtable_newfd()`. `fdstack_search()` only reuses a struct already on
   the *exact current* level, and L2 has nothing yet for slots 1/2/3, so
   three brand-new structs get allocated at L2 (call them S₃, S₁, S₂ for
   the order they're created in) rather than reusing L1's or L0's. Each
   is a lazy alias (`fd_dup()`'s pointer copy, `d->r = dupe->r` etc.) of
   whatever currently tops that slot: S₃ aliases L1's `$X` struct, S₁
   aliases L1's `$Y` struct, S₂ aliases **S₃** (which itself only
   resolves down to `$X` once *its* alias is chased) — a live, is
   multi-hop alias chain, entirely within L2, never actually `dup2()`d
   for real anywhere in this sequence under the *original*, unmodified
   code (that's precisely the bug being chased — it stays lazy).
4. `echo a`/`echo b >&2` write straight through those alias pointers —
   correctly, in the unmodified code, since nothing has torn any of the
   aliased-*from* structs down yet.
5. `eval_subshell()` returns: pops L2. `fd_pop()`/`fd_close()` runs on
   S₃/S₁/S₂ — harmless, since none of them own a real descriptor of their
   own (pure pointer aliases, `rb.fd`/`wb.fd` were never set).
6. `eval_command()` finishes: pops L1. *Now* the real, fd-owning structs
   for `$X` (5) and `$Y` (6) get `fd_close()`d for real — correctly, since
   the whole compound command is done with them.

That sequence is self-consistent in the original code, matching the
observed-correct output. The crash-inducing change was forcing step 3's
aliases to resolve *eagerly*, via `fdtable_dup(..., FDTABLE_FORCE)`, the
moment each `exec` redirection runs — which mutates the *global*
`fd_expected`/`fd_list[]`, not anything scoped to L2. Because there's no
`fork()` bounding "the subshell" at the OS level, that mutation isn't
undone by L2's pop in step 5 the way the *rest* of the subshell's state
(vartab, functions, traps, the fdstack level itself) already is — it
leaks into step 6 and beyond, corrupting bookkeeping the *parent* script
relies on for whatever runs after the subshell returns. Same underlying
gap as the pipeline breakage above (code that assumes "this is safe,
nobody else needs this fd number" without actually checking), just
triggered through eager resolution instead of through the buffer_close
guard removal — either path reaches the same kind of unscoped, global
mutation.

### Suspected problems (ranked, most load-bearing first)

1. **No fdstack-scoped ownership tracking.** `fd_expected`, `fd_list[]`,
   `fd_top`/`fd_lo`/`fd_hi` are all single global variables, but a
   persistent redirection's "this fd is free now" is only actually true
   for *however long the current process lives* — which is unbounded
   inside a non-forking subshell, since "the current process" outlives
   the subshell scope that logically owns the mutation.
2. ~~**`fdtable_gap()`'s unsafe branch has no invariant documented, let
   alone enforced**~~ — **fixed 2026-08-17**, see "Progress" below.
3. **Persistent (`exec`) redirections and eval_subshell's in-process
   model are fundamentally in tension.** `fd_new()`/`fdtable_newfd()`'s
   docs/comments talk about "persistent" as if it always means
   "for the rest of the process" — true at the real top level, false
   inside a non-forking subshell, and nothing in the code distinguishes
   the two cases.
4. ~~**The fd ≤ 2 protection is a blanket workaround, not a fix**, for the
   fact that fds 0/1/2 are simultaneously (a) real OS resources that can
   legitimately need closing/reopening (what `exec` wants) and (b)
   implicitly assumed-immortal by a large fraction of `src/fd*` written
   before/without that in mind.~~ — **resolved 2026-08-18**: the
   workaround itself is gone (`buffer_close()`'s guard dropped to plain
   `fd >= 0`), replaced by real per-struct ownership checks in
   `fd_close()` instead of a blanket fd-number cutoff. See "Current
   status" and the third-pass "Progress" below.

### Suggested refactorings

- ~~**Scope `fd_expected`/`fd_list[]` mutations to the fdstack level that
  caused them**~~ — **done 2026-08-17** for `eval_subshell()`, see
  "Progress" below. Confirmed *necessary but not sufficient* — problem 3
  turned out not to be subshell-exclusive (see "Progress").
- **Give persistent redirections real subshell-awareness.** Either (a)
  make `eval_subshell()` genuinely fork when it contains a persistent
  (`exec`) redirection anywhere in its body — expensive to detect
  up-front, but sidesteps the whole shared-global-state problem by
  construction — or (b) teach `fdtable_newfd()`/`fd_close()` that a
  "persistent" redirection created inside a pushed-for-a-subshell
  fdstack level is only persistent *for that level's lifetime*, and
  should behave like a temporary one for teardown purposes.
- ~~**Make `fdtable_gap()`'s eviction safe unconditionally**: always
  `fd_setfd(gap, -1)` before `fd_pop(gap)` regardless of `NOCLOSE`, and
  have the caller's later `dup2()`/`open()` re-establish the number.
  This turns "was this call site given NOCLOSE" from a load-bearing
  correctness requirement into a pure optimization hint, closing off
  the whole class of bug problem 2 describes.~~ — **done 2026-08-17**
  (unconditional neuter) **and extended 2026-08-18** (relocate via
  `dup()` instead of destroy, plus the same fix applied to
  `fdtable_dup()`'s `dup2()` branch — see "Current status").
- ~~**State the fd ≤ 2 invariant explicitly, in code, not just in
  `buffer_close()`'s behavior.** A `fd_is_owned(struct fd*)`-style check
  (real ownership, per `fd_list[e] == fd`, not just "is the number ≤ 2")
  at the point `fd_close()` decides whether to actually close, so the
  policy lives next to the fd table that understands ownership, not in a
  generic `libowfat` buffer primitive shared with unrelated code that
  has no such table to consult.~~ — **done 2026-08-18**: `fd_close()`
  now checks `fd_list[]` against `rb.fd`/`wb.fd` directly before every
  real `close()` call (not wrapped in a separate named helper, but the
  same substance — see `src/fd/fd_close.c`).
- ~~**Once ownership is real, drop the blanket fd ≤ 2 guard from
  `buffer_close()` entirely** (restoring it to plain upstream
  `libowfat` behavior) — `fd_close()` itself is where the "should this
  actually close" decision belongs; the low-level buffer primitive
  shouldn't be the one enforcing a shish-specific policy that doesn't
  apply to its other callers (`path_gethome.c`, `mmap_filename.c`).~~
  — **done 2026-08-18**, see "Current status" above.

*(Historical note, true as of the 2026-08-17 writing of this paragraph,
no longer true now: at that point the `buffer_close()`/`fd_close()`
side had been reverted every time it was tried, and `redir_dup.c`'s
eager-dup2() change had never been kept either. As of 2026-08-18, the
`buffer_close()`/`fd_close()` side above **is** implemented and kept —
see "Current status". `redir_dup.c`'s eager-dup2() change landed on
2026-08-21 as `fixes/193`; what follows was written while it was still
reverted/not retried, per "Current status".)*

### Progress (2026-08-17, second pass)

Two of the refactorings above are now actually implemented and merged
into the working tree, landed independently of the still-unsafe
`buffer_close()` change:

- **`fdtable_gap()`'s eviction is now unconditionally safe**
  (`src/fdtable/fdtable_gap.c`): `fd_setfd(gap, -1)` always runs before
  `fd_pop(gap)`, not just under `FDTABLE_NOCLOSE`; only the return value
  (`e` vs. `FDTABLE_DONE`) still varies by that flag. Verified against
  `tests/fixed.sh` (385/388, identical to baseline — the 3 failures are
  pre-existing and unrelated), the full `ctest` suite, and
  `exec-p.tst`/`redir-p.tst`/`pipeline-p.tst` (byte-identical pass/fail
  counts to baseline across repeated runs). Safe and neutral on its own,
  as expected — it only matters once something can actually reach real
  fd ≤ 2 through the branch it fixes, which the current `buffer_close()`
  still prevents.
- **`fd_expected`/`fd_top`/`fd_lo`/`fd_hi`/`fd_list[]` are now
  snapshotted and restored around `eval_subshell()`**, mirroring
  `vartab_push()`/`vartab_pop()` exactly: `struct fd_state`
  (`src/fd.h`) plus `fd_state_save()`/`fd_state_restore()`
  (`src/fd/fd_state_save.c`/`fd_state_restore.c`), called immediately
  after `fdstack_push(&io)` and immediately after `fdstack_pop(&io)` in
  `eval_subshell.c`. Also verified regression-clean the same way as
  above. **What this does not fix, by design** (stated up front in
  `fd_state_restore()`'s own comment): it undoes shish's *bookkeeping*
  about which real fds are free, not any actual `dup2()`/`close()`
  syscall a persistent redirection inside the subshell already issued
  against a real, still-live descriptor — a real kernel fd, once
  changed, stays changed no matter what the bookkeeping says afterward.

**Re-tested the original two experimental changes (`buffer_close()`'s
guard removed, plus eager `fdtable_dup()` resolution in
`redir_dup.c`) together with both fixes above in place** to see how much
of the original blast radius they close off. Result: the segfault
repro (the stdout/stderr-swap subshell) is gone — 10/10 clean runs,
where it was reliably reproducible before. But **the pipeline breakage
is not fixed, and turned out not to be subshell-related at all**:
`X=$(echo hi | sed 1q)` (no subshell anywhere) still fails with `Bad
file descriptor` once enough state has accumulated earlier in a long
script. Given this, all three of the original experimental changes
were reverted again for that pass — see below for what actually closed
this off.

### Progress (2026-08-18, third pass — root-caused and fixed)

Picked the pipeline-breakage thread back up using `strace -f -e
trace=fork,clone,dup,dup2,close,pipe2,execve` against the real
(non-debug) binary instead of shish's own `DEBUG_FDTABLE` trace — the
shell's own bookkeeping agreed with itself right up to the actual
`close()` syscall, so the bug was only visible by watching the real
syscalls a forked child issues immediately before its own `execve()`.
That showed a pipeline's later stage (e.g. `sed`/`rev` in
`echo x | sed 1q`) getting its own, just-established real stdin
`close()`d again, right before `execve()`, in the *same* child process
— `fd 0` simply missing from `/proc/<pid>/fd` by the time the program
ran, so it either hung (spinning on `read()`+`EBADF` in a program that
doesn't check for it — a `rev` process was found already stuck like
this from an earlier run, 300+ CPU-minutes in) or printed "Bad file
descriptor" directly.

Root cause: `fdstack_flatten()` (`src/fdstack/fdstack_flatten.c`,
called in every forked child right before `execve()` to reap any
struct that's `!= fdtable[fd->n]`, i.e. shadowed) calls `fd_pop()`
unconditionally on what it finds shadowed — including a struct whose
real fd number (`->e`, or independently `rb.fd`/`wb.fd` — see below)
has, by that point, been silently reused by a *different*, currently-
active struct after an earlier eviction elsewhere failed to relocate
it (the same failure mode fixed for `fdtable_gap()` in the previous
pass, just reached through two more call paths). Confirmed by
instrumenting `fdstack_flatten()` to dump both the victim and
`fdtable[fd->n]` (the "top" struct) via `fd_dump()`: the victim (the
process's own inherited stdin struct, `"char device"`) and the
top/active struct (the pipeline's own `"pipe"` struct) both showed
`e=0` — two distinct `struct fd*` objects both claiming to own the
same real kernel fd 0.

Two more instances of the same "unconditionally destroy, don't check
who else might already own this real fd" bug, beyond the
`fdtable_gap()` one fixed last pass:

- **`fdtable_dup()`'s `dup2()` landing branch**
  (`src/fdtable/fdtable_dup.c`): when `flags & FDTABLE_CLOSE` is set,
  `fdtable_wish()` (and so `fdtable_gap()`) is deliberately skipped —
  see its own comment, "if we can close the destination there's no
  need to wish" — and `dup2(o, d->n)` runs straight over whatever
  currently occupies real fd `d->n`. `dup2()` closes that atomically
  in the kernel *before* this function ever gets a chance to look at
  it, so unlike `fdtable_gap()`'s case, there is no "undo the close"
  option here — the occupant has to be relocated via a fresh `dup()`
  *before* the `dup2()` call, not after. Fixed by checking
  `fd_list[d->n]` immediately before the `dup2()` call and relocating
  a merely-shadowed occupant there, mirroring `fdtable_gap()`'s
  already-fixed branch.
- **`fd_close()`'s real `close()` calls** (`src/fd/fd_close.c`): the
  actual bug hit by `fdstack_flatten()` above. The existing
  `fd_list[fd->e] == fd` check only guarded the *bookkeeping* clear
  (`fd_list[fd->e] = 0`) — it never gated whether `buffer_close()`
  actually got called. Worse, that check is keyed on `fd->e`, which
  can be `-1` (a struct whose buffer sits on its own real fd —
  stralloc/here-doc temp files — independently of ever becoming the
  struct's own "effective" descriptor; see the existing comment right
  above it) while `rb.fd`/`wb.fd` still hold a live real number. Fixed
  by checking `fd_list[]` against `rb.fd`/`wb.fd` directly,
  unconditionally (not gated on `fd->e` or `FD_DUP` mode), right
  before the `buffer_close()` calls: if some other struct is already
  the registered owner, neuter this struct's copy (`= -1`) instead of
  closing it for real.

With both of these plus last pass's `fdtable_gap()` fix in place,
`buffer_close()`'s guard was narrowed for good (`fd >= 0`, not `fd >
2`) — the fd≤2 special case is gone from `lib/buffer/buffer_close.c`
entirely, restoring it to a plain, policy-free primitive as suggested
in "Suggested refactorings" above. Verified:

- The `Bad file descriptor`/hung-`rev` repro (`echo hi | sed 1q` after
  enough `mktemp`/`mkdir`/heredoc churn) is clean 10/10 runs, confirmed
  both minimally and via a truncated `tests/fixed.sh` prefix.
- The stdout/stderr-swap subshell repro from last pass is still clean
  10/10 (no regression from this pass's changes).
- `tests/fixed.sh` in full: same 3 pre-existing, unrelated failures as
  baseline (`main`, no session changes) — `the same holds for a
  pipeline that actually pushes multiple lines through the builtin`
  and the two `set +h` cases — confirmed by literally stashing this
  session's fd/fdtable changes, rebuilding, and diffing the failure
  list.
- Full `ctest` (including `tests/posix/*.tst`): 61/142 failed on both
  this session's tree and a stashed-back baseline tree, byte-identical
  failure list (`diff` of the two sorted `(Failed)`/`(Timeout)` lines
  was empty). The signal-related failures (`sigcont*`/`sighup*`/
  `sigint*`/`sigquit*`/`sigterm*`/`sigurg*`) are a property of running
  without a controlling terminal in this environment, not a regression.
- Regression tests added to `tests/fixed.sh` (fixes/186, fixes/187);
  patches in `fixes/186-fd-table-bookkeeping-vs-real-close-desync.patch`
  and `fixes/187-subshell-fd-table-not-scoped.patch`.

**Done 2026-08-21** (`fixes/193`): the eager-`fdtable_dup()`-in-
`redir_dup.c` approach was retried on this firmer ground and kept,
with one addition — it is skipped inside a subshell
(`!exec_subshell_depth`), because that is where it still breaks; see
problem 3 above, which now has a concrete repro because of it.

---

## Goal 5 (secondary) — make the binary smaller (musl and dietlibc are the targets)

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
produces (5.1, done); the right one adds LTO, `--icf=all` and `-no-pie`,
which are still opt-in. The dietlibc row is not a typo -- a *static*
diet build undercuts the old *dynamic* glibc one. It was blocked on
the `HAVE_WINSIZE` probe missing `<termios.h>`; fixed 2026-08-22
(`fixes/199`), and the 152072 figure above is the confirmed result.

### 5.1 Free wins: build flags, no source change -- **done 2026-08-22**

Stacked, in order of what each one buys on the glibc dynamic build:

```
baseline                                              189312
-fno-asynchronous-unwind-tables -fno-unwind-tables    152448   -19%
  -fno-stack-protector -fno-ident
+ -no-pie -fno-pie                                    144744
+ LTO                                                  140256
+ -ffunction-sections -fdata-sections --gc-sections    136152   -28%
  -Wl,--icf=all -Wl,--build-id=none
```

`.eh_frame` + `.eh_frame_hdr` alone are 28452 of the 189312 baseline
bytes -- 15% of the binary, for a shell that never unwinds. That is the
single biggest item on this list and it costs one flag.

Notes from measuring:

- LTO alone: 189312 -> 172392. `-ffunction-sections`/`--gc-sections`
  alone: 189312 -> 185216 (little dead code to collect; the builtin
  table references every builtin, so nothing there is collectable).
- `--icf=all` needs gold or lld. **gcc `-flto` + `ld.lld` is broken**
  (lld cannot read GCC bitcode: `undefined symbol: main`), and it fails
  *at configure time*, so every `check_include_file` silently reports
  "not found" and the build then dies somewhere unrelated. Use gold
  with gcc; lld only with clang.
- The tuned build passes the same `tests/*.sh` as the untuned one
  (21/23; `builtin-rmdir.sh` and `fixed.sh` fail identically on both).

**What landed.** `MinSizeRel` now probes and applies every flag it can
(`cmake/Checks.cmake`, new `check_ldflag()` in `cmake/Functions.cmake`):
`-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector
-fno-jump-tables -fno-plt -fno-ident -fmerge-all-constants
-ffunction-sections -fdata-sections`, and `-Wl,--gc-sections
--as-needed --build-id=none -z norelro -z noseparate-code
--hash-style=gnu`. Each is probed before use, so a toolchain that lacks
one just skips it. On top, `strip_minsize()` (`CMakeLists.txt`) runs
`strip -s -R` over `.comment`, `.note*`, `.eh_frame` and `.eh_frame_hdr`
after each MinSizeRel link -- worth 29240 bytes on its own, even with
no flag changes at all. `-DMINSIZE_STRIP=OFF` turns that off.

Verified: `tests/*.sh` gives the same 21/23 as before on glibc and on
musl-static (`builtin-rmdir.sh` and `fixed.sh` already fail on `main`);
`Debug` and `Release` binaries are byte-for-byte what they were.

Still opt-in, and not in the numbers above: LTO (`-DENABLE_LTO=ON`,
worth ~8%), `--icf=all` (needs gold or lld), and `-no-pie` (drops
`.rela.dyn`, at the cost of ASLR for the executable).

### 5.2 Help and usage text: ~13 KB of a 136 KB binary

`.rodata` is 18662 bytes, and the 38 `help_*` strings are 10234 of
them -- 55%. On top: 848 bytes of usage strings and a 1760-byte
`builtin_table` in `.data.rel`. Roughly 10% of a tuned binary is text
that only `help` and usage errors ever print.

Wanted: `-DENABLE_HELP_TEXT=OFF` that nulls the `help`/usage fields of
`struct builtin`. Disabling the `help` *builtin* does not help today --
`builtin_table.c` names every `help_*` symbol, so they all link anyway.

Related, smaller: packing the two `char*` fields into offsets in one
string blob removes 56 relocations from `.data.rel.ro`.

### 5.3 Stop dragging libc subsystems in for one caller each

Measured in the musl static build:

| symbol pulled in | bytes | why | replacement |
|---|---|---|---|
| `pow` (+ libm) | 1916 | `A_EXP` in `expand_arith_binary.c:51` | integer `**` loop -- shell arithmetic is integer, so `pow()` is also a correctness hazard |
| `glob` + `do_glob` + `fnmatch_internal` | ~5200 | `expand_glob.c:61` | the shell already has `path_fnmatch` (1564 bytes); glob = readdir + that |
| `__qsort_r` | 991 | `term_complete.c:60`, sorting completions | insertion sort over a handful of names |

`lib/unix/glob.c` exists but is `#if WINDOWS_NATIVE` only, so every
Unix build takes libc's.

### 5.4 Re-decide the `LINK_STATIC` mem-routine switch per libc

`lib/byte.h:60` maps `byte_copy`/`byte_zero`/... to `memcpy`/`memset`/...
when linking dynamically, and uses the in-tree loops in `lib/byte/` when
linking statically -- to avoid pulling glibc's enormous `memcpy` into a
static binary. Measured on musl, that trade is a wash and slightly
backwards:

```
musl static, in-tree byte_* (today)   text 222589
musl static, libc mem*                text 222069
```

`memcpy`/`memset`/`memcmp`/`strlen` are linked **either way** -- the
compiler emits calls to them for struct copies and initializers, so the
`#if` never actually keeps them out; it just adds a second, slower copy
of each. glibc-static is the only case the switch was right about
(1181200 bytes, dominated by libc). So: condition it on the libc, not on
the link mode, and keep the fast libc routines everywhere except
glibc-static.

### 5.5 Builtin set

`-DENABLE_ALL_BUILTINS=ON` costs 26 KB over the default set
(215232 vs 189312 stripped). The `EXTRA_BUILTINS` group (`cat`, `chmod`,
`ln`, `rm`, `mkdir`, `mktemp`, `uname`, ...) is what the container and
agent-sandbox pitch is built on, so it is not obviously droppable -- but
a documented "what does each builtin cost" table would let a distroless
image pick. Largest single builtins, text+data of the object:
`trap` 3987, `test` 3824, `printf` 3778, `expr` 3216, `set` 3120.

### 5.6 Not binary size, but on the same pitch: 262 KB of `.bss`

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

## Also open (secondary)

- **Line-editing/terminal-abstraction/key-bindings rewrite** — a
  design-sized project inherited from the old `TODO` file, not a fixable
  bug. Minimal filename tab-completion (`src/term/term_complete.c`) is the
  only piece of this done so far.
