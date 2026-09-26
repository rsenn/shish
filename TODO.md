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

**Stale:** the `sig*-p` family is down ~500, to ≈67 (the four `*2-p`
files plus `sigurg2-p`/`sigcont2-p` are 180/180). The full scoreboard has
not been re-run from a clean `ctest`; the per-file numbers below (and the
`sig*` breakdown after them) still reflect the older state. The signal
files are noisy (see the warning below); the 6103 skips are not passes,
see Phase 6.

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

### Phase 1 [Stage 1: language] — signal disposition (≈100 left, was 1790)

Measured 2026-09-27 on an idle machine: every `*2-p` file is 0 failures; `sighup6` 3, `sigint6`/`sigquit6`
12, `sigint5`/`sigquit5` 15, `sigint1`/`sigquit1` 11, `sigterm5`/`sigterm6` 6, the rest 3 each.
Done: an interactive shell ignores INT/QUIT/TERM for itself and resets them in forked children,
`exec`, `(...)` and `$(...)` (`sh_sigignore()`/`sh_sigrestore()`); `trap - SIG` in an interactive shell
resets a signal that was ignored on entry.

What is left:

1. **Asynchronous lists** (`cmd &` without job control): POSIX says INT and QUIT are ignored in the
   background child. `job_fork()`/`exec_program()` do not do this; it is most of the remaining
   "spares child/shell (async, ...)" cases.
2. **`trap ... INT` inherited across `exec`/subshell** ("command -> keep" cases: a trap must
   become the default in a child, an ignore stays ignored).
3. **Interactive `kill -s INT $$` in the main shell** with an initially-ignored INT
   (`sigint1`/`sigquit1`: 11 cases each).

### Phase 2 [Stage 1: language] — error semantics: which failures must exit the shell (0)

What is left of this subject is diagnostics only: shish prints
`file:LINE:COL: msg` where the line number is one too high (the parser
has already advanced) and omits the offending name. `echo ${x?boom}` on
line 2 reports `:3:1: boom`; bash reports `line 2: x: boom`. `$LINENO`
itself is correct. Fix with, and verify against, `lineno-p.tst` (2/3)
and `BUGS: error-message-line-number-off-by-one`.

---

### Phase 3 [Stage 2: builtins/utilities] (≈100 failures, each step independent)

Sorted by failures per unit of work.

1. **`alias` (22/65).** What is left is not fixable in `builtin_alias.c`
   at all — see `BUGS: alias-substitution-needs-rework-in-parse_gettok`,
   and `BUGS: quote-backslash-escaping-broken` for the one remaining
   quoting-side failure (`reusing printed alias (complex quotation)`).
   Note: the remaining `kill1-p`/`kill2-p` failures are not a shish bug
   — `bash`'s own `testee() ( … exec "$testee" "$@" )` wrapper prints a
   job-control notice ("Aborted (core dumped)", …) to the redirected
   stderr whenever the exec'd process dies from a signal, and `dash` run
   through the same harness fails the identical set.
2. **`read` (22).** IFS whitespace vs non-whitespace splitting,
   backslash continuation, reading no more than one line.
   `BUGS: read-field-splitting-and-options-broken`.
3. **`command` (15)** — `-v`/`-V` output formats for builtins,
   externals with and without a slash; and a not-found dot script
   must not kill the shell.
4. **`unset` (6)** — `-f` (functions) does not delete; readonly
   variables must not be deletable.
5. **`umask` (9)**, **`set` (8)**, **`shift` (4)**, **`export` (1)**.
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

1. **6103 of 12195 cases are skipped**, mostly the 44 `%REQUIRETTY%`
   files (the `sigttin`/`sigttou`/`sigtstp`/`sigstop` `*3-p`/`*7-p`/`*8-p`
   combos, `kill4-p`, `bg-p`/`fg-p`/`job-p`, `testtty-p`, `wait-p`), which
   need a real controlling terminal. `-DDO_PTY_TESTS=ON` runs them under
   `tests/pty-run.c` (a single-file POSIX-`pty` wrapper). Result: **10/44
   pass** (`testtty-p` and the `kill`-driven `*4-p` combos), 34 fail —
   some are ordinary conformance gaps (e.g. `sigcont3-p`'s 3 output
   mismatches), but most of the `*3-p`/`*7-p`/`*8-p` combos plus
   `wait-p`/`kill4-p` hang until the 60s `pty-run` alarm — a real
   job-control defect, since their `kill`-driven `*4-p` siblings pass in
   1-2s. One case is traced: `BUGS: wait-interrupted-by-trap-hangs`
   (`wait` doesn't get interrupted by an arriving trapped signal). The
   rest need per-case bisection — `BUGS:
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

Sorted by whether the `BUGS` entries below explain part of the
`tests/posix` gap above, or are unrelated maintenance items:

**Directly explains a scoreboard number (fix these as part of Stages 1-2):**

- `signal-tests-vary-with-machine-load`, `kill-stop-self-in-subshell-deadlock`
  → Phase 1 (`sig*-p`, `kill3-p`).
- `error-message-line-number-off-by-one` → Phase 2 (`lineno-p`).
- `alias-substitution-needs-rework-in-parse_gettok`, `read-field-splitting-and-options-broken`,
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

- `eval-lineno-imprecise-inside-function`,
  `heredoc-in-cmdsub-read-in-wrong-order`,
  `exit-trap-loses-positional-parameters`,
  `help-builtin-columns-overlap`, `no-tree-print-option-is-a-noop`,
  `cfg-cmake-mingw-silently-builds-native`, `eval-node-bgnd-silent-on-fork-failure`.

**Memory safety, not conformance** — tracked under "Memory safety" below
instead: `asan-leak-residue-not-fully-triaged`,
`ubsan-buffer-op-proto-function-type-mismatch`,
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

1. `asan-leak-residue-not-fully-triaged` — three call sites untriaged
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
3. `builtin-fork-races-sh-onsig-sigchld` — a real fork/signal race
   (not just a sanitizer artifact); most likely to surface as a
   flaky, hard-to-reproduce ASan failure during Stage 1/2 work rather
   than its own dedicated session, so flag it here rather than let it
   get blamed on whatever fix was running at the time.
4. **Goal 4** below (fd/fdtable/redir vs. non-forking subshells) is
   also a memory-safety item, not just a conformance one — its "Still
   open" problem has a confirmed heap-corruption repro. See that
   section for the full writeup.

---

## Everything below this line is lower priority than the MAIN QUEST above.

---

## Goal 3 (secondary) — arena allocator for the AST (`lib/arena` done, not wired into `src/` yet)

`src/tree.h`'s AST is a graph of individually `malloc()`'d nodes
(`tree_newnode()`) plus separately `malloc()`'d string buffers hanging off
several of them — one `malloc`/`free` pair per node, even though a tree's
real lifetime is always "parse it all at once, evaluate, throw the whole
thing away" (`sh_loop.c`). `lib/arena.h` + `lib/arena/` now implement a
generic, shell-independent bump allocator (tested by `tests/arena_test.c`,
nothing in `src/` uses it yet):

```c
arena_init(a, &arena_heap|&arena_mmap|&arena_brk, chunk); /* pluggable source */
arena_init_fixed(a, buf, len);            /* stack array, alloca(), .bss */
arena_alloc(a, size, align);              /* zeroed, NULL when full, never exits */
arena_new(a, T);  arena_newn(a, T, n);    /* size/alignment/overflow in the macro */
arena_dup(a, p, len);  arena_strndup(a, s, len);   /* frozen blobs */
arena_grow(a, p, old, new);  arena_trim(a, p, old, new); /* newest only; grow is NULL otherwise */
arena_tell(a);  arena_rewind(a, pos);     /* nested lifetimes */
arena_reset(a);  arena_free(a);
```

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
  `narg.stra` stays a real `stralloc` — it's populated later, at
  expansion time, not parse time. Packing a node and its string tightly
  adjacent in the arena is safe with no alignment padding, since
  `src/tree.h`'s node structs are already `__packed`.
- **Expansion results are a separate problem.** Words expand into `N_ARG` field nodes on the
  heap, not into the parse arena; see Goal 3b for the field-list redesign that follows this one.
- **Possible future: precompiled/cached AST on disk.** Serialize arena
  blocks with node pointers rewritten to offsets; on load, run one linear
  fixup pass turning offsets back into real pointers (structured like
  `tree_free()`'s own `switch(node->id)`) — after that, every existing
  tree-walking function works completely unmodified. A more invasive
  "offsets natively everywhere, zero-copy `mmap()`" design is possible but
  touches every tree-walking call site for a benefit unlikely to matter
  next to lexing/parsing cost.

---

## Goal 3b (secondary, after Goal 3) — word expansion into a field list, not N_ARG nodes (planned, not started)

Depends on Goal 3: nothing here starts until the parser allocates from an arena.

**What expansion is.** `src/expand/` works only on *words*: an `N_ARG` whose `list` holds
`N_ARGSTR`/`N_ARGPARAM`/`N_ARGCMD`/`N_ARGARITH` parts. The result is a chain of `N_ARG`
nodes, one per field, each carrying a growable `stralloc` (`narg.stra`). Consumers only ever
need `(char* s, len)` per field plus one bit ("keep this field even if empty",
`X_QUOTED|X_NOSPLIT|X_SPLIT`): `expand_argv()` for argv, `eval_for.c` for the loop
variable, `expand_vars()` for assignments (always exactly one field). Several entry points
(`expand_str/tostr/tosa/copysa/catsa`, `expand_range`; used by case, redirections, here-doc
delimiters, prompts, `${x:-word}` messages) want a single concatenated string and no fields at
all; `expand_copysa`/`expand_catsa` fake that with a `union node tmpnode` on the stack.

**Measured (2026-09-20, `valgrind ./shish -c ...`, 1000 loop iterations, control loop
`[ $i -lt 1000 ]; i=$((i+1))` subtracted).** Per execution of one simple command:

| command                  | mallocs | bytes |
| ------------------------ | ------- | ----- |
| `:`                      | 5       | 214   |
| `: abcdefgh` (one word)  | 10      | 436   |
| `: a b c d e f g h`      | 45      | 1927  |
| `: $i`                   | 10      | ~440  |
| `: ${y:-z}`              | 13      | ~535  |

So a literal word costs **5 mallocs, ~215 B**. `set -- $(seq 1 20000); : "$@"` adds
60,029 mallocs / 2.34 MB, about **3 mallocs and 117 B per field** for a ~6-byte string.

**Where the 5 mallocs per word go.**
1. `expand_args()` and `expand_vars()` `tree_copy()` the whole argument tree on *every*
   execution (3 mallocs per word: `N_ARG`, `N_ARGSTR`, its string) so `expand_brace_args()` and
   `expand_tilde_word()` may rewrite it. Almost no word contains `{` or a leading `~`.
2. The result is one `N_ARG` node (48 B packed + malloc header) and one `stralloc` buffer sized
   `len + len/8 + 30` (`lib/stralloc/stralloc_ready.c`), so short fields waste most of it.
   `expand_cat()` has seven copies of "`tree_newnode(N_ARG)` + `stralloc_init`".
3. Fields have no lifetime of their own: they live until `tree_free(args)`, after the command.

**Design (each step is its own change).**

- **Stage 1: stop copying (done 2026-09-20: `expand_brace_needed()`, `expand_tilde_needed()`,
  `expand_tilde_assign_needed()`; `eval_case` too; measured 5 -> 2 mallocs per word).** Add read-only predicates ("does this word have a `{` chunk / a
  leading `~`?") and `tree_copy()` only when some word needs rewriting; otherwise expand the
  permanent tree directly and skip `tree_free(owned)`. 5 -> 2 mallocs per word. Independent of
  the arena.
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

  Design:
  ```c
  struct xlist { stralloc blob; size_t* off; size_t n, a; stralloc cur; unsigned state; };
  void xl_cat(struct xlist*, const char*, size_t, int flags); /* = expand_cat's state machine */
  void xl_break(struct xlist*);   /* close the open field; the next cat opens a new one */
  char** xl_argv(struct xlist*);  /* NUL-terminated strings, argv-shaped; keep/drop already decided */
  ```
  Closed fields are NUL-terminated strings appended to one `blob`, with their offsets in one
  vector that `xl_argv()` turns into `char*` in place: the result *is* `argv`, with no
  per-field node or flag. The open field is built in `cur` (glob/unescape happen when it
  closes, in one place). The "keep an empty unquoted field" decision (`X_SPLIT`) is made when
  the next field opens or the word ends, so no flag survives. Single-string entry points
  (`expand_str/tostr/tosa/copysa/catsa`, case, redirections, prompts, `expand_arith_expr`)
  make `cur` the caller's `stralloc` and skip fields entirely, which deletes `tmpnode`.
  Two mallocs per command (blob, vector) instead of two per word, reusable per nesting level.
  `expand_cat()`'s state machine keeps its logic (it encodes many `fixes/NN`); only its state
  moves out of `narg.flag` into `xlist.state`, and the seven "new node" blocks become one
  `xl_open()`.
  Pointer stability: `blob` may move while building, so offsets are used until `xl_argv()`;
  afterwards it is fixed for the life of the command. Command substitution and function calls
  nest, but each level owns its own `xlist`, so nothing shares a moving buffer.
  Zero-copy option: a word that is one unquoted literal chunk without backslash or glob
  characters could put a pointer straight to the parse-tree string in the vector (parse strings
  are already NUL-terminated by `stralloc_nul()` in `parse_string.c`); valid while the tree is
  alive, i.e. for the command's duration.
  Not `stralloc[]` with flags in `.a`: `stralloc_ready()` treats `a != 0` as capacity, so a flag
  value there would look like allocated space; the existing alias convention is `a == 0`.
- **Rule: a field-rewriting step must run at finalization, never as a later pass.** Once the
  arena backs `blob` (Stage 4), any step that changes an already-closed field's size (glob is
  the existing example; a brace-on-expansion-result extension, which shish doesn't have --
  bash/dash/shish all leave `a='bl{a,e,i}h'; echo $a` unexpanded, confirmed 2026-09-21 -- would
  be another) can only extend that field in place while it is still `top` of the arena, i.e.
  before the next field opens. `xl_close()`/`expand_glob()` already run exactly there. A rewrite
  applied as a second pass over an already-built list, after a later field has been appended,
  finds the target is no longer `top`; `arena_grow()` correctly refuses it (see `lib/arena.h`),
  and the only fallback is an O(n) shift of everything after it in `blob` -- no cheaper than the
  `realloc()`-based copy the arena was meant to avoid.
- **Arena scoping (only once Goal 3 lands).** `blob` and the vector become arena allocations
  taken between `arena_tell()` and `arena_rewind()` in `eval_simple_command`, `eval_for` and
  `expand_vars`; with a contiguous sbrk arena (Stage 4) `blob` grows in place and `cur` can live
  at its tail. Until then plain `alloc()` is fine and is not an arena move.
- **Side effect on the parse tree.** `narg.stra` is used only by expansion, so `struct narg`
  drops its 24-byte `stralloc`: parse-time `N_ARG` nodes shrink from 48 to 24 bytes.
- **Stage 3 (optional): read-only tilde and brace.** Tilde can be applied when `expand_arg`
  sees a word's first literal chunk (`expand_tilde_lookup()` already returns home + prefix
  length). Brace expansion generates whole alternative words today, so it would become "expand
  the word once per alternative with chunk k replaced". After this `tree_copy()` is only used
  for function and trap bodies.
- **Stage 4 (optional): contiguous sbrk arena.** If `arena_brk` chunks are contiguous the arena
  can extend `end` instead of chaining; then `arena_grow()` never fails and `cur` could live at
  the arena tail (obstack style), removing the copy on close.

**Expected result:** ~2 mallocs per command instead of 2 per word, about length+1+8 bytes per
field instead of ~117 B, no `tree_free()` of expansion results, no `narg.stra`, and about ten
fewer `tree_newnode()` call sites.

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

## Goal 4 (secondary, but overlaps MAIN QUEST memory safety) — `fd`/`fdtable`/`fdstack`/`redir`: persistent (`exec`) redirections vs. the non-forking subshell model

One problem is left from a long investigation of the fd table (bookkeeping
drift in `fdtable_gap()`, `fdtable_dup()`, `fd_close()`, `fdstack_flatten()`
is all fixed). The unit that scopes fd state is the *subshell environment*
— an in-process scope that does `sh_push()`, of which there are exactly two:
`eval_subshell()` and `expand_command()` (`$(...)`). A new in-process
subshell scope has to repeat the same six saves (fdstack, fd_state, vartab,
env, functions, traps).

### Still open

**Persistent (`exec`) redirections vs. the non-forking subshell model are
fundamentally in tension.** `eval_subshell()` runs `(...)` in-process, and
`fd_new()`/`fdtable_newfd()` can't distinguish "persistent for the rest
of the process" (true at the real top level) from "persistent only for
this subshell's lifetime" (true inside a non-forking subshell). Nothing
in the code distinguishes the two cases. `fd_state_save()`/
`fd_state_restore()` cover the *bookkeeping* half (enough to prevent the
segfault), but do nothing about real `dup2()`/`close()` syscalls a
persistent redirection inside a subshell already issued against a
still-live descriptor before the subshell returns. `redir_dup()`'s eager
`fdtable_dup(nredir->fd, FDTABLE_FORCE | FDTABLE_CLOSE)` therefore has to
be skipped inside a subshell (`!exec_subshell_depth`).

Concrete demonstration: drop that guard and

```sh
( exec 3>&1 1>&2 2>&3 3>&- ; echo hi ) >/dev/null 2>&1
/bin/true
```

prints `fdtable: redirection cycle detected` from the forked child at
`fdtable_exec()` time, and segfaults outright deeper into a longer script
(`tests/fixed.sh` dies at its own `fixes/73` swap case). At the point of
the error `fdtable[1]` has `n=1, e=4` while the real fd 1 is held by
another struct that also wants slot 1, so `fdtable_gap()` recurses into
resolving it and trips the cycle check.

The obvious reading — "the subshell's real `dup2()`s outlived it while
`fd_state_restore()` put the bookkeeping back" — is **not the whole story,
and fixing only that is not enough.** Tried and rejected: a per-scope
pre-image list (`fd_scope_park()` called just before `fdtable_dup()`'s
`dup2(o, d->n)`, parking the old descriptor with `fcntl(F_DUPFD, 64)`,
restored with the owning `fdtable[]` entry on scope exit). It fires where
expected — 4 parks for the swap repro above — and is harmless with the
guard in place, but with the guard removed the cycle error is *unchanged*
and heap corruption follows: the leftover state that trips `fdtable_gap()`
is in the fd table's own entries and `fd_list[]`, not merely in the kernel
descriptors. Parking *every* open fd at scope entry (rather than lazily,
per overwritten number) deadlocked the suite — an extra copy of a pipe's
write end keeps its reader from ever seeing EOF. Do it with the fd table
in front of you:

```sh
cmake -S . -B build/dbg -DCMAKE_BUILD_TYPE=Debug \
      -DDEBUG_FDTABLE=ON -DDEBUG_FD=ON -DDEBUG_FDSTACK=ON
build/dbg/shish -c 'exec 3>&1; dump -t'   # -t table, -s stack, -f list
```

### Suggested refactoring

**Give persistent redirections real subshell-awareness.** Either (a) make
`eval_subshell()` genuinely fork when it contains a persistent (`exec`)
redirection anywhere in its body — expensive to detect up-front, but
sidesteps the whole shared-global-state problem by construction — or
(b) teach `fdtable_newfd()`/`fd_close()` that a "persistent" redirection
created inside a pushed-for-a-subshell fdstack level is only persistent
*for that level's lifetime*, and should behave like a temporary one for
teardown purposes.

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
produces; the right one adds LTO, `--icf=all` and `-no-pie`, which are
still opt-in. The dietlibc row is not a typo -- a *static* diet build
undercuts the old *dynamic* glibc one.

### 5.1 Remaining opt-in build flags

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

**Plan: an internal POSIX `glob`, and a `USE_LIBC_GLOB` option (2026-09-19).**
Policy for everything shish re-implements that libc also has: use libc's
where it costs nothing (dynamic linking) *unless* the libc function has no
`(ptr, len)` form and we need one — then the internal one is always used.
`fnmatch(3)` is the example: NUL-terminated only, so `path_fnmatch` stays
internal for `case`, `${x%pat}` and (Goal 6) regex bracket sets.

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
   `expand_glob.c` needs no change beyond the include it already selects
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
   patterns finally use **one** matcher (the open `case-pattern-*` /
   `fnmatch-quotation-*` entries in `BUGS` are about expansion and
   quoting, not about which matcher runs, so this does not fix them).
4. **Verification:** a dev-only differential script over a fixture tree
   (dotfiles, brackets with classes, escaped metacharacters, symlinks,
   unreadable directories) comparing the internal backend with libc
   `glob64` in the C locale, plus `tests/` cases for what POSIX 2.13
   specifies (`/` never matched by `?`/`*`/`[...]`, leading period matched
   only explicitly, results in collation order, unmatched pattern left
   as is — `expand_glob.c` already handles the last). Must pass in both
   `USE_LIBC_GLOB` settings; ASan+UBSan gate as usual.
5. Interaction with Goal 7: with the internal `glob`, `?` and `[...]` in
   *pathname* patterns become UTF-8-aware for free once `path_fnmatch` is
   (M3), and the `setlocale` question in Goal 7 (C) disappears for
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

## Goal 6 (secondary) — `lib/dfa/`: one POSIX regex engine for `expr`, `grep`, `sed`

**Built, but not the way this section plans — see the discrepancy note
below.** `expr :` and a new `grep` both landed on it; the rest of this
section (BRE/ERE scope, conformance checklists, size estimates) is
still an accurate read of what the engine has to do, just not of how
its execution strategy or file layout ended up.

**Discrepancy (2026-09-24):** built as `text/dfa/` (public header
`text/dfa.h`, prefix `dfa_`), not `lib/dfa/` — `text/` didn't exist
when this plan was written; `CLAUDE.md` later drew the line that a
subsystem this large and shell-specific (not a generic portable
primitive) belongs there instead of `lib/`. More importantly, the
*execution strategy* is not the "lazy DFA with a bounded, `cbmap`-backed
state cache" the Architecture diagram below describes: it's Pike's
thread-based NFA simulation (`dfa_run.c`, one pass, no state caching)
for backref-free patterns, falling back to an explicit-stack
backtracker (`dfa_bt.c`) only when a pattern actually needs
back-references. Both give POSIX leftmost-longest matching and safe
bounded memory without ever caching or interning NFA-state subsets, so
**the `cbmap` state-interning use case in the Architecture section does
not apply to what was built** — there is no lazy-DFA/subset-construction
mode to cache. If one gets added later purely as a speed optimization,
that's where `cbmap` (or `hashmap`, see the Goal 11 hashmap-vs-cbmap
decision) would actually come in; until then `cbmap` has no consumer in
this codebase. `dfa_prefix`/`dfa_submatch`/`dfa_search` all exist and
match this section's API sketch.

**Not started (still applies): `sed`.** The rest of the original plan
follows, describing the shared regex engine both `expr`/`grep` already
use and `sed` still needs.

Naming: `dfa_` and not `re_` because glibc's `<regex.h>` declares
`re_search`/`re_match`/`re_compile_pattern` under `_GNU_SOURCE`. The
directory holds a backtracker as well (captures and backrefs are
impossible in a pure DFA); the DFA is the primary engine and gives the
directory its name.

### Scope

- POSIX BRE and ERE, C locale, bytes (no UTF-8 for now; `scan_utf8`/
  `fmt_utf8` in `c-utils/lib` are the way in later). Subject strings are
  `(ptr, len)`, never NUL-terminated, so NUL bytes are ordinary data.
- No GNU extensions by default (`\w \s \b \< \>`, `\d`). `\| \+ \?` in a
  BRE are *implementation-defined* in POSIX.1-2024 (Chapter 9), so they
  are the one place a `DFA_GNU` flag is cheap and legitimate (see "Regex
  language: decisions"). Everything else GNU stays out.
- Bytes-only is conformant: POSIX.1-2024 requires only the POSIX locale
  (§6.2: "256 single-byte characters") and never mentions UTF-8. It is
  wrong in a UTF-8 locale (`.`, `[é]`, `y`, `l`, `expr` lengths are
  character-based there); shish never calls `setlocale`, so it is always
  in the C locale. A later `DFA_UTF8` flag would compile sets to UTF-8
  byte-sequence automata (est. +200-300 lines); see Goal 7, which
  plans the whole shell-side switch (`WITH_UTF8`).
- Non-goals: Perl syntax, lookaround, lazy quantifiers, locale collation
  (`[.ch.]` multi-character elements), approximate matching.

### What was surveyed and why nothing is adopted as-is

| Candidate | Size / licence | Why not |
|---|---|---|
| `ag.c` (IOCCC, SirWumpus) | 307 lines, K&R+macros | ERE subset, no `+`/`{}`/`[:class:]`/backrefs; `(a*)*b` segfaults (no visited set in the closure); `^.` matches an empty line (the `\n` sentinel leaks into `.`); `fgets` splits long lines; `d[512][512]` = 1 MB bss. Good *algorithm*, not reusable code. |
| Russ Cox `dfa1.c` | ≈500 lines, MIT header (the index page says "All Rights Reserved" — check the file) | yes/no only; no anchors/classes/extents. Useful reference for the bounded, flushable state cache. |
| onetrue-awk `b.c` | ≈1800 lines, Lucent permissive | closest feature match (ERE, classes, `{n,m}`, `pmatch` = leftmost-longest start+length) but tangled into awk (`FATAL`, `tostring`, UTF-8 runes, global `patbeg`/`patlen`); no captures, no BRE. Reference for the leftmost-longest search loop. |
| TRE / musl `regcomp.c`+`regexec.c` | ≈4400 lines, BSD-2 (`musl-1.2.4` is in `~/Projects`) | full POSIX incl. submatch rules, but not a DFA and drags wide-char/locale code. Useful as a *test oracle*. |
| libregexp (`plot-cv/quickjs`) | ≈72 KB linked | JavaScript semantics: leftmost-first alternation, `\` escapes inside `[...]`; needs a translator plus a patch for longest-match. Rejected. |
| GNU `dfa.c`, gnulib regex | GPL, gnulib-entangled | too big; licence version unchecked against `COPYING` (GPLv2). |

shish itself is GPLv2 (`COPYING`), so MIT/BSD/Lucent-style code could be
folded in; the plan is nevertheless to write it fresh from the
algorithm, with these borrowings: flat integer program + `.*` prefix for
unanchored search (`ag.c`), bounded cache that flushes when full
(Cox), leftmost-longest by anchored runs from each start (awk `pmatch`).

### Architecture

```
pattern ─▶ lex ─▶ parse ─▶ program (flat int array, Thompson NFA)
                                  │
              ┌───────────────────┼────────────────────┐
              ▼                   ▼                    ▼
        lazy DFA            backtracker          submatch pass
   (test / prefix /       (backref patterns:    (fills \1..\9 inside
    search: yes/no and     the whole match)      a span the DFA found)
    match extents)
```

- Bracket expressions are *not* parsed by `lib/dfa`: the compiler calls
  `path_fnmatch(bracket, len, &c, 1, PATH_FNM_NOESCAPE)` for `c = 0..255`
  once and stores a 256-bit set. Matching is a bit test; `-i` sets the
  other case's bit; negation inverts the set. The lexer only has to find
  the closing `]` (skipping `[:x:]`) and swap a leading `^` for `!`.
- Anchors: `dfa_test` may run over `"\n" + line + "\n"` so `^`/`$` are
  ordinary byte matches (`ag.c` trick), but only if `.` and negated sets
  exclude `\n`. sed needs real anchors (a pattern space can contain
  `\n`), so `^`/`$` also exist as program ops; the DFA treats them as
  context-dependent epsilon edges.
- DFA state = sorted array of program indices; interned through a
  key→id map (`cbmap`, key = the array's bytes); transitions cached per
  state; cache bounded (default 64 states, flush and restart when
  full) — never a fixed `[512][512]` table, no `.bss` (Goal 5.6).
  **Not what got built** (see the discrepancy note at the top of this
  goal): `text/dfa` runs Pike's thread simulation instead, with no
  state cache and no `cbmap` involved.
- Patterns with backrefs skip the DFA (`d->backrefs`); the public
  functions route to the backtracker, invisibly to callers.
- Multiple patterns (grep `-e a -e b`, newline-separated lists) compile
  as one alternation, so a whole `-e`/`-f` set is one DFA.

### Regex language: decisions (POSIX.1-2024 Chapter 9, read 2026-09-19)

Grammar recap, so the lexer/parser can be written without the spec open.
`glibc` results below are from `regcomp`/`regexec` on this machine
(2026-09-19) and are the oracle only where the spec is silent.

| Construct | BRE | ERE | Decision |
|---|---|---|---|
| ordinary char, `\` + special char | yes | yes | literal |
| `.` | any char | any char | any byte incl. `\n` in `sed`; grep never sees `\n` inside a line. NUL is an ordinary byte (the spec says a pattern may not contain NUL; a *subject* may, since we use `(ptr,len)`) |
| `[...]` | yes | yes | see "Bracket expressions"; `[.ch.]`/`[=e=]` accept single characters only |
| `*` | after an atom; **literal** at the start of the RE, after `\(`, after a leading `^` | after an atom; leading `*` undefined | BRE: as spec. ERE: literal at start / after `(` / after `\|` (GNU and busybox agree) |
| `\{m\}` `\{m,\}` `\{m,n\}` | yes | `{m}` `{m,}` `{m,n}` | `m <= n <= 255` (`RE_DUP_MAX`, POSIX minimum). Larger → error. ERE `{` not followed by a digit: **error** (glibc: error; spec: undefined; a literal `{` is what GNU grep does, kept out to stay simple) |
| grouping | `\( \)` | `( )` | capturing, numbered by `(` position, max 9 addressable by `\1`-`\9`; more groups allowed, just not back-referenceable |
| alternation | **not in the spec** (`\|` is implementation-defined) | `\|`→`|` | ERE yes. BRE `\|` `\+` `\?`: error by default, accepted with `DFA_GNU` |
| `+` `?` | not in the spec | yes | ERE yes; BRE only with `DFA_GNU` |
| `^` | anchor only at the start of the RE or after `\(` (and `\|` with `DFA_GNU`); literal elsewhere | anchor everywhere | as spec; `a^b` in ERE can never match |
| `$` | anchor only at the end of the RE or before `\)` | anchor everywhere | same |
| back-reference `\1`-`\9` | yes | **undefined** | supported in both (glibc and GNU grep do; `(a)\1` matches `aa` there). Reference to a group that is not yet closed → error (`DFA_ESUBREG`) |
| adjacent duplications `a**`, `a*+` | undefined | undefined | accepted; `X**` is collapsed to `X*` at parse time (glibc accepts and gives `[0,3)` on `aaa`), so it cannot create an exponential empty loop |
| empty alternative `a|`, `(|a)`, `()` | — | undefined | accepted, matches the empty string (glibc accepts `a|`); needed for `(|a)` idioms |
| empty RE | — | — | matches the empty string at every position. In grep: matches every line. In sed: means *the last regex used at run time* (sed only, resolved by the builtin, not by the library) |
| `\n` in a pattern | undefined | undefined | a literal newline byte; sed's `\n` escape is turned into a newline by sed's own parser before the pattern reaches `dfa_compile` |

**Bracket expressions.** The compiler finds the closing `]` (a `]`
first in the list is a member; `[:x:]`, `[.x.]`, `[=x=]` are skipped as
units), rewrites a leading `^` to `!`, and asks `path_fnmatch` byte by
byte to fill a 256-bit set (see Architecture). Backslash is an ordinary
member inside brackets in a RE, so pass `PATH_FNM_NOESCAPE`. Range
endpoints are compared as byte values (the C locale); a reversed range
`[z-a]` is an error in POSIX-2024 ("undefined") and yields the empty
set in `path_fnmatch` — make the compiler check it. Unknown class name
`[[:foo:]]` → error. `-i` folds both cases into the set at compile time,
so the matcher never knows about case.

**Limits** (constants at the top of `lib/dfa.h`, tested at the boundary):

| Limit | Value | Why |
|---|---|---|
| pattern length | 32 KiB (`DFA_MAXPAT`) | `-f` files can be large; a list of a few thousand words must compile |
| program instructions after `{m,n}` expansion | 32 767 (`DFA_MAXPROG`) | `a{255}{255}` must not allocate 65 025 copies of anything; checked with `umult32` before copying |
| groups | 32 recorded, 9 addressable | matches the fixed `\1..\9` syntax; extra groups still group |
| nesting depth of `( )` | 256 | bounds parser recursion, so `((((…` × 100 000 is an error, not a stack overflow |
| DFA states cached | 64 (settable, tests use 3) | see "Matching" |

**Error codes** (`dfa_error(code)` returns a fixed string, no allocation):
`DFA_ENOMEM`, `DFA_EPAREN` (unbalanced), `DFA_EBRACKET`, `DFA_EBRACE`
(bad/unclosed interval, `m > n`, > 255), `DFA_ERANGE` (reversed range),
`DFA_ECLASS`, `DFA_ESUBREG` (bad back-reference), `DFA_EBADRPT`
(repetition of nothing, ERE), `DFA_EESCAPE` (trailing `\`), `DFA_ESIZE`
(program or nesting limit). The builtins print
`grep: <message>` / `sed: -e expression #1, char N: <message>` and exit 2
(grep) or 1 (sed); `expr` maps every compile error to status 2.

**Sub-match rule — a known deviation.** POSIX requires each
subexpression, left to right, to match the longest possible string
consistent with the overall match being longest; a repeated group
records its last iteration. The backtracker enumerates alternatives in
preference order (greedy, leftmost alternative first) and keeps the
first path that ends at the span `dfa_search` found. That equals the
POSIX answer except where a shorter earlier subexpression still lets the
overall match reach the same end. Measured on glibc 2026-09-19:
`(a|ab)(bc|c)?` on `abc` → glibc reports `g1=[0,1) g2=[1,3)`, whereas the
strict rule gives `g1=[0,2) g2=[2,3)`; so glibc has the same deviation
and it is not worth being stricter than the platform. Test suite
consequence: group expectations are only asserted where the answer is
unambiguous (see "Testing").
Empty iterations: a loop iteration that matches the empty string ends
the loop and is not recorded unless it is the first — `\(a*\)*` on `aa`
gives `\1 = aa` (glibc: `[0,2)`), `(a*)+` on `b` gives `[0,0)`.

### Program format and matching (how the pieces fit)

**Program.** One `int` array, instruction = `op | arg << 4`:

| Op | Arg | Meaning |
|---|---|---|
| `CHAR` | byte | consume one byte equal to arg |
| `ANY` | — | consume any byte |
| `SET` | index into the set table | consume a byte whose bit is set; sets are 32 bytes each, deduplicated by content |
| `SPLIT` | x | try next instruction first, then x (order = greediness: `a*` is `L: SPLIT end; CHAR a; JMP L`) |
| `JMP` | x | goto |
| `SAVE` | 2·group + {0,1} | record position (backtracker only; DFA treats it as a no-op epsilon) |
| `BOL` `EOL` | — | zero-width; true at offset 0 / at n (unless `DFA_NOTBOL`) |
| `BACKREF` | group | consume a copy of the recorded text (backtracker only) |
| `MATCH` | — | accept |

`{m,n}` is expanded by copying the body's instructions `m` times plus
`n-m` optional copies (nested `SPLIT`s so an empty body cannot loop);
`X{0}` emits nothing but still numbers its groups. An unbounded tail is
one `*` loop. This is the only place a pattern grows, hence the
`DFA_MAXPROG` check.

**Closure.** `dfa_closure(prog, pc, flags, set, mark)` follows `JMP`,
`SPLIT`, `SAVE`, and `BOL` (only when the flag says the context is a
line start) using an explicit stack and a per-call `mark[]` bitmap. The
bitmap is the entire fix for the `(a*)*b` crash in `ag.c`: a pc is
expanded at most once per closure, so empty loops terminate.
`EOL` and `MATCH` are kept in the set as leaves (`EOL` becomes live only
in the end-of-input check).

**Lazy DFA.**
- State = sorted array of the leaf pcs (`CHAR/ANY/SET/EOL/MATCH`) that
  the closure reached; interned in a `cbmap` keyed by the array's bytes.
  Id 0 is the dead (empty) state, and never leaves the cache.
- Byte classes: at compile time all `CHAR` values and `SET`s partition
  the 256 bytes into `ncls` equivalence classes (`a` `b` `c` inside `[a-c]`
  share one). `state->next[ncls]` therefore is small (often < 20 ints),
  `-1` = not computed yet. One extra class column means "end of input".
- Flags per state, computed at creation: `accept` (MATCH in the set)
  and, lazily, `accept_at_end` (MATCH reachable through `EOL` leaves).
- Two start states, cached: `at_bol` and `not_bol` (what `^` means differs;
  `dfa_search(..., from > 0)` and `DFA_NOTBOL` use the second).
- Cache bound: when the state count reaches the limit the whole cache
  (map, states, transition rows) is freed except the current state, which is
  re-interned, and matching continues. Correctness never depends on the
  cache; the test suite forces it with a limit of 3 states.
- Total memory is O(limit × (ncls + pattern)), never O(text).

**Entry points, precisely.**
- `dfa_test`: unanchored. Runs the DFA from the `at_bol` start, and before
  each byte also injects the start pcs (the `.*` prefix of `ag.c`, done as
  "add start set to every state" rather than as extra instructions, so the
  same program serves anchored and unanchored). Stops at the first
  `accept`; a match at end-of-input needs `accept_at_end`. No captures.
- `dfa_prefix`: anchored at `s[0]`, continues until the dead state or the
  end, remembers the last accepting offset; that is the longest match
  (POSIX `expr :`).
- `dfa_search(from)`: (1) `dfa_test` over `s+from` to reject cheaply — a
  line with no match costs one linear pass, the common grep/sed case;
  (2) skip to the first byte that can start a match (first-byte set,
  `dfa_prefilter`) and run `dfa_prefix` at each candidate start until one
  succeeds — the first success is leftmost, and `dfa_prefix` makes it
  longest. Worst case O(n²) per call (e.g. `a*b` on `aaaa…`); a reverse
  DFA would remove that and is deliberately not planned.
  `s///g` calls it in a loop with `from` = end of the previous match
  (+1 after an empty match) and the context flag `not_bol` from the
  second call on.
- `dfa_submatch(span)`: runs the backtracker over `[span.start, span.end]`
  and requires the path to *end exactly* at `span.end`. Because the span
  is already known to be a match, the search is a directed walk, not a
  blind one; for patterns without back-references it still can blow up
  only on adversarial nested groups, so it is bounded by a step budget
  (`DFA_MAXSTEPS`, 10 million) after which the groups are reported unset.
- Patterns with back-references have no DFA: `dfa_test/prefix/search`
  call the backtracker, which explores all paths and keeps the longest
  end at each start.

### API (each function its own file)

```c
struct dfa;                                      /* compiled pattern + lazy state cache */
struct dfa_span { size_t start, end; };

int    dfa_compile(struct dfa*, const char* pat, size_t len, unsigned flags);
                       /* flags: DFA_ERE  DFA_ICASE  DFA_NOTBOL  DFA_LIST (newline-separated alternatives)
                                  DFA_GNU (BRE \| \+ \?) */
void   dfa_free(struct dfa*);
size_t dfa_groups(const struct dfa*);            /* number of \( \) groups */
const char* dfa_error(int code);

int    dfa_test(struct dfa*, const char* s, size_t n);
           /* grep, sed /re/ address: is there any match? */
long   dfa_prefix(struct dfa*, const char* s, size_t n);
           /* expr ':': longest match anchored at s, -1 if none */
int    dfa_search(struct dfa*, const char* s, size_t n, size_t from, struct dfa_span* m);
           /* sed s///g, grep -o: leftmost-longest at or after `from`; `^` still means offset 0 */
int    dfa_submatch(struct dfa*, const char* s, size_t n, const struct dfa_span* m,
                    struct dfa_span* g, size_t ng);
           /* fill \1..\9 inside a match dfa_search already found */
```

`grep` needs only `dfa_test`; `expr` needs `dfa_prefix` (+`dfa_submatch`
for a `\(...\)`); `sed` needs `dfa_test` for addresses and
`dfa_search` + `dfa_submatch` in a loop for `s///`. POSIX `grep` has no
`-o`, so only `expr` and `sed` need leftmost-longest.

### API contract

- `struct dfa` is caller-allocated (`static struct dfa re;` in the
  builtin); `dfa_compile` allocates its internals, `dfa_free` releases
  them, and calling `dfa_free` on a zeroed struct is a no-op. On a
  compile error the struct is left freed/zeroed, so the caller never
  needs to clean up after a failure.
- Subjects are `(const char* s, size_t n)`; the library never reads
  `s[n]` and never writes to `s`. Offsets in `struct dfa_span` are
  relative to `s`, `end` exclusive, `start == end` is an empty match.
- `dfa_test` returns 1/0; `dfa_prefix` returns the length or `-1`
  (`0` is a valid empty match); `dfa_search` and `dfa_submatch` return
  1/0 and fill their out-parameters only on 1. Out of memory while
  matching (state cache growth) is handled by flushing, never by failing
  the call; the only `DFA_ENOMEM` is at compile time.
- `dfa_submatch` groups that did not participate get
  `{(size_t)-1, (size_t)-1}` so `sed`'s `\3` can tell "unset" (empty
  replacement) from "empty match".
- A `struct dfa` is not thread-safe (the lazy cache is mutated by matching)
  — shish is single-threaded, so this is only documented.
- `DFA_ICASE` folds only ASCII in the first pass; with `WITH_UTF8` (Goal 7)
  it stays ASCII-only for sets (documented deviation, same as classes).

### Reuse from `lib/` and `../c-utils/lib`

Static archive, so an unreferenced file costs nothing in the binary;
still copy only what is used. `CMake` globs `lib/*/*.c`
(`cmake/libowfat.cmake`), autotools needs a `Makefile.in` per new dir.

| Need | Take | State in shish's `lib/` |
|---|---|---|
| unbounded streaming line reader | `buffer_getline_sa`, `buffer_get_token_sa` (c-utils `lib/stralloc/`, 7+29 lines) | declared in `lib/stralloc.h:145-151`, **`.c` missing** (link error if called) |
| growable program / state table | `array` (`array_allocate/get/length/truncate/reset`, ≈7 files; needs `likely.h`, `safemult.h`, `typedefs.h`) | absent |
| state interning map | `cbmap` (12 files; has `cbmap_destroy` = cache flush). `hashmap` is smaller (164 lines) but has no free-all. | absent |
| `grep -F` | `byte_findb`/`byte_finds` (22 lines), `case_findb` for `-i` | absent |
| pattern-list split, first-byte prefilter | `byte_chrs`, existing `byte_chr` | `byte_chrs` absent |
| line boundaries in a memory window | `scan_lineskip`, `scan_eolskip` (≈ `byte_chr(..,'\n')+1`) | absent |
| overflow-checked `{m,n}` expansion | `umult32` (`safemult.h`, header only) | absent |
| interval / escape numbers | `scan_ulongn`, `scan_8long`, `scan_fromhex`, `scan_xchar` | present |
| sed `l` command | `fmt_escapecharcx`, `fmt_escapecharnonprintable` | absent |
| later multibyte | `scan_utf8`, `fmt_utf8` | absent |
| bracket expressions | `path_fnmatch` (see above) | present |

### File layout and size estimate

Line counts are in **this repo's style** (clang-format, one function
per file, header comment per function — roughly 40% blank/comment/brace
lines). Calibration, measured: the current `expr` BRE matcher is 255
lines for BRE-only, no intervals, no backrefs; `builtin_expr.c` is 434
lines and 3216 B of object code, `printf` 446 lines / 3778 B,
`test` 392 / 3824 B — so ≈ 7-10 bytes of code per source line. An
earlier rough figure of 350-700 lines for the whole library was in
compact lines and too low; the itemised total is below.

| Group | Files | Lines |
|---|---|---|
| API and plumbing | `lib/dfa.h` 90, `dfa_compile` 60, `dfa_free` 20, `dfa_error` 30, `dfa_emit` 50 | ≈250 |
| lexer + parser | `dfa_lex` 90, `dfa_lex_bracket` 60, `dfa_lex_interval` 40, `dfa_parse_list` 30, `dfa_parse_alt` 30, `dfa_parse_seq` 30, `dfa_parse_repeat` 90, `dfa_parse_atom` 80 | ≈450 |
| NFA closure/step | `dfa_closure` 70, `dfa_step` 40 | ≈110 |
| lazy DFA | `dfa_state` 70, `dfa_next` 60, `dfa_cache_flush` 25, `dfa_run_test` 50, `dfa_run_prefix` 50, `dfa_search` 70, `dfa_prefilter` 40 (optional) | ≈365 |
| backtracker + captures | `dfa_bt` 60, six node matchers ≈150, `dfa_submatch` 60 | ≈270 |
| **`lib/dfa/` total** | ≈27 files | **≈1450 (range 1100-1700), ≈11-13 KB code** |
| imported from c-utils | `array` ≈7 files, `cbmap` 4-5 used files, `byte_findb`, `byte_chrs`, `buffer_getline_sa`, `buffer_get_token_sa` | ≈300 lines copied, not written |

| Source file | Today | Estimate | Notes |
|---|---|---|---|
| `src/builtin/extra/builtin_expr.c` | 434 (matcher ≈255) | **≈210** | matcher deleted, ≈30 lines of glue. −225 lines. The operator layer (`index`, string comparison, `\|`, `&`) is separate work: `BUGS: expr-index-wrong-result`, `expr-string-comparison-numeric-only`, +100-150 lines if done properly. |
| `src/builtin/builtin_grep.c` | — | **≈300** (250-350) | option parse ≈40, pattern collection (`-e`, `-f`, list split) ≈60, per-file loop with `file:` prefix ≈60, output/`-c -l -n -q` ≈50, `-F` path ≈30, help text ≈30. ≈2.5-3 KB. busybox `grep.c` is 944 lines but has the GNU option set and uses libc regex. |
| `src/builtin/builtin_sed.c` | — | **≈850** (700-1000; revised in Goal 11, was ≈1200) | script parser ≈400, executor/cycle ≈400, `s` command (replacement compile, flags, `&`/`\N`) ≈200, `r`/`w` files and text commands ≈120, options/help ≈80. ≈10-12 KB. busybox `sed.c` is 1684 lines with GNU extensions. |
| `tests/dfa.sh`, `builtin-grep.sh`, `builtin-sed.sh` | — | ≈200 / 150 / 300 | table-driven; each ends with `summary` |

Binary-size context (Goal 5): the whole `ENABLE_ALL_BUILTINS` set costs
26 KB today; `lib/dfa` + `grep` + `sed` is roughly **+25 KB when all
three are on**. `expr` is in `DEFAULT_BUILTINS`, so moving it onto the
library grows the *default* build by the parser + backtracker
(≈970 lines ≈ 8 KB) minus the old matcher (≈2 KB) ≈ **+6 KB (≈4 %)**
unless `dfa_prefix`/`dfa_submatch` are kept off the DFA objects (the
linker then drops them). Measure after step 3 and decide (see below).

### POSIX conformance checklists (what "done" means; spec text read 2026-09-19, POSIX.1-2024)

**`grep`** — `grep [-E|-F] [-c|-l|-q] [-insvx] -e pattern_list ... [-f pattern_file] ... [file...]`
and `grep [-E|-F] [-c|-l|-q] [-insvx] pattern_list [file...]`.
POSIX.1-2024 lists exactly `-E -F -c -e -f -i -l -n -q -s -v -x`; Issue 8
adds nothing else, so `-o -w -r -h -H -A/-B/-C` are extensions (`dfa_search`
makes `-o` cheap; decide after the core is done, not before).
- Patterns: `-e`/`-f`/first operand are each a list separated by newlines;
  the union of all lists is the pattern set, compiled as *one* alternation
  (`DFA_LIST`). An empty pattern in the set matches every line. `-E`
  = ERE, `-F` = fixed strings (each list line a literal; `-x`/`-i` apply),
  default = BRE. `-e`/`-f` may be mixed and repeated; `-f` on an empty
  file adds no patterns (nothing matches; with `-v` everything does).
- Input: each `file` in order; `-` or no operand = stdin. A line is
  everything up to `\n` (or EOF without one); NUL bytes are data; lines of
  any length (streaming `buffer_getline_sa`, no fixed buffer).
- Selection: `-v` inverts; `-x` requires the match to span the line
  (`^(...)$` around the alternation, or `dfa_prefix == n`); `-i` = `DFA_ICASE`.
- Output: default the whole line + `\n`; **the `file:` prefix only when
  more than one file operand**; `-n` = `lineno:` before it (after the file
  prefix); `-c` = count per file (`file:count` for several files);
  `-l` = file names only, one per line, each file listed once and reading
  of that file stops at the first match; `-q` = no output, stop at the
  first match anywhere; `-s` = suppress messages about nonexistent or
  unreadable files. For stdin with a prefix the label is
  `(standard input)` (GNU, busybox; POSIX leaves it open).
- Exit status: 0 = some line selected, 1 = none, >1 = error. With `-q`,
  0 even if an error occurred (as long as a line matched). An unreadable
  file is an error (status 2) but the remaining files are still processed.
- Not to be assumed: `--` handling comes from `shell_getopt` (`grep -e -x`
  and `grep -- -x file` must both work); `-e` argument beginning with `-`.
- Extras only if cheap: `egrep`/`fgrep` names (dispatch on `argv[0]` like
  `digest`); still open, POSIX 2024 removed them.

**`sed`** — `sed [-n] [-E] script [file...]`, `sed [-n] [-E] -e script ... [-f script_file] ... [file...]`.
- Options: `-n`, `-e`, `-f`, and **`-E` (ERE) which POSIX.1-2024 added**
  (so it is *not* an extension any more, contrary to the first draft of
  this plan). `-r` is the GNU spelling of the same; accept it as an alias
  (free). `-i` is *not* in POSIX 2024; `-s`, `-z` are not either: out.
  Multiple `-e`/`-f` join with a newline in the order given. `#n` as the
  first two characters of the *first* script line (only if exactly `#n`
  followed by newline) acts as `-n`.
- Commands (function letter, address count): `{ } = a b c d D g G h H i l
  n N p P q r s t w x y : #`. `=` writes the line number + `\n`. `}` may
  be preceded by `;`. Labels: a `:label` and `b`/`t` label read to end of
  line (POSIX: `;` does **not** terminate a label — but GNU does and the
  common one-liner `:a;N;$!ba;s/\n/ /g` relies on it; measured GNU sed
  4.x 2026-09-19 accepts it. **Decision:** terminate labels at `;` and
  whitespace like GNU; a label with `;` is not valid POSIX use anyway).
- Addresses: none / one / two (`addr1,addr2`); a number, `$` (last line
  of the last file, or of each file with `-s`, which we do not have —
  last line of the whole input), `/re/`, `\cREc` (custom delimiter; the
  delimiter inside the RE means itself), and `addr!cmd` (spaces after
  `!` allowed). A range whose end address is a number ≤ the start line
  matches one line. If addr2 is a regex it is tried starting with the
  line *after* addr1. Empty regex = **the last RE used at run time**
  (not compile time): keep a `struct dfa* last`.
- `s/re/repl/flags`: replacement `&`, `\1`-`\9`, `\n` for newline
  (from the *script* level), `\&` and `\delim` literal, backslash-newline
  = newline. Flags: `g` (all), a decimal `n` (only the nth), `n` with `g`
  (nth and after), `p` (print if replaced), `w file` (write if replaced),
  **`i`/`I` (case-insensitive, added in Issue 8)**. Empty-match rule
  with `g`: after an empty match copy one byte and advance so `s/x*/-/g`
  on `abc` gives `-a-b-c-`; a non-empty match directly after an empty
  one is allowed only if it starts later (`s/b*/-/g` on `abc` gives `-a-c-`).
  `t` flag "any successful substitution since the last input line was read
  or last `t`".
- Text commands: `a\`/`i\`/`c\` text: the text starts on the next line;
  each line but the last ends with `\`; leading blanks of text lines
  are stripped unless preceded by `\` — this follows POSIX, GNU's one-line
  `a text` form is accepted too (same code path: text after `a` on the
  same line). `a` and `r` output is queued and written at the end of the
  cycle, or when the next line is read (`n`, `N`). `i` writes at once. `c`
  deletes the pattern space and starts a new cycle; in a range it prints
  once at the end of the range (with `!` it prints for every line).
- Files: `r file` (missing file silently ignored); `w file`: **created
  or truncated when the script is parsed, before any input is read**, and
  the same name in several commands shares one open file; `/dev/stdout`
  is a special name (write to fd_out, not a new open).
- Execution: `n` with no next input branches to the end of the script
  and quits (autoprint happens; re-read the spec wording when
  implementing, only `N` was verified here). `N` — **with no next input,
  quit without printing** (spec text; measured 2026-09-19: GNU sed
  prints the pattern space by default and prints nothing under
  `POSIXLY_CORRECT=1`; busybox prints). We follow POSIX and document that
  GNU-written scripts such as `sed N` on odd-length input differ.
  `D`: if the pattern space has no newline act like `d`, else delete up to
  the first newline and restart the cycle **without** reading input.
  `G`/`H` append a newline plus the other space; `x` swaps; `q` prints
  (unless `-n`) and exits, exit code argument is an extension (GNU) but
  free to take.
- `l`: long form, line folded to the terminal width (POSIX: implementation-
  defined; use 70 columns, wrapping with `\` at the end of the output
  line), escapes `\\ \a \b \f \n \r \t \v`, other non-printables as
  3-digit octal `\ooo`, `$` at the end of every line (`fmt_escapecharcx`
  from c-utils covers most of it).
- `y/abc/xyz/`: equal lengths, `\\`, `\n`, `\delim` escapes; a 256-byte
  table built at parse time.
- Output: **every line written is terminated by a newline**, also a last
  input line that lacked one (spec: input and output are text files). GNU
  sed preserves the missing newline (measured: `printf 'a\nb' | sed p`
  → `a\na\nb\nb` with no final newline). Decision: follow POSIX (always
  newline) — simpler, and a decision to reverse later is a one-flag change.
- Limits from the spec to honour, not to restrict: a script may have
  `w` files of any number ≥ 10; text of any size.
- Non-goals (first pass): `e`, `F`, `z`, `W`, `R`, `M` flag, `0,/re/`,
  `addr,+N`, `first~step` (GNU extensions).

**`expr`** — `expr expression`; every argument is a token.
- Grammar, lowest precedence first (all left associative): `expr1 | expr2`,
  `expr1 & expr2`, `= > >= < <= !=`, `+ -`, `* / %`, `expr1 : expr2`,
  then `( expr )` and operands.
- `|`: `expr1` if it is neither null nor `0`, else `expr2` if that is
  neither, else `0`. `&`: `expr1` if both are neither null nor `0`, else
  `0`. Comparison: **numeric if both are integers (an optional sign and
  digits), else string collation** (bytes in the C locale) — this is the
  bug `expr-string-comparison-numeric-only`. Arithmetic: signed long
  integers; overflow is undefined, division by zero is an error.
- `:`: `string : BRE`, anchored at the start (as if `^`), result = the
  number of bytes matched, or, if the BRE has `\(...\)`, the text of the
  *first* group. **No match → `0` (no group) or the null string (with a
  group).** `^` at the start of the BRE is redundant but legal.
- Exit status: **0** the result is neither null nor `0`; **1** it is null
  or `0`; **2** invalid expression; **>2** an error occurred (division by
  zero, unwritable stdout). Today's `expr` gets these wrong: see
  `BUGS: expr-crashes-and-exit-status`.
- `--`: the argument `--` ends options (POSIX utility guideline 10), so
  `expr -- -1` prints `-1` (today: SIGSEGV).
- `length`, `substr`, `index`, `match`: **not POSIX** (Issue 8 does not
  define them). They exist today (`BUGS: expr-index-wrong-result`); keep
  them, fix `index`, do not extend the set.
- Migration to `lib/dfa` (step 3 below): `builtin_expr.c` keeps its
  operator layer; only `expr_match()` (the ≈255-line BRE matcher) becomes
  `dfa_compile(&re, pat, len, 0)` + `dfa_prefix` (+ `dfa_submatch` if
  `dfa_groups(&re) > 0`).

### Registration and build checklist (per new builtin: grep, sed)

1. `src/builtin/builtin_<name>.c` with `const char help_<name>[]` and
   `int builtin_<name>(int argc, char* argv[])`, options via
   `shell_getopt` (same shape as `builtin_digest.c`).
2. `src/builtin.h`: prototype and `extern const char help_<name>[]`.
3. `src/builtin/builtin_table.c`: the fallback `#define BUILTIN_<NAME> 0`
   and the table entry (`{"grep", &builtin_grep, help_grep, …}`), ordered
   alphabetically like its neighbours.
4. `cmake/Builtins.cmake`: name in `EXTRA_BUILTINS` (off by default).
   `lib/dfa/*.c` is picked up by the `lib/*/*.c` glob; the builtin needs
   nothing more, but `expr` in `DEFAULT_BUILTINS` will make `libowfat.a`
   supply `dfa_*` (static archive: unreferenced members cost nothing).
5. Autotools: `configure.ac` builtin list and a `Makefile.in` in `lib/dfa/`.
6. `tests/builtin-<name>.sh`, skipped when the builtin is not compiled in
   (same `type` check as `tests/builtin-digest.sh`).
7. Docs: one line in the README builtin list; `--help` text kept short
   (Goal 5.2 counts help bytes).

### Testing strategy

- **`tests/dfa.sh`** (CTest). Needs a C-level driver, since the library is
  not reachable from the shell before `expr :` moves onto it; until then
  test through `expr :` and, once it exists, `grep`/`sed`. Table rows are
  `flags|pattern|subject|expected`, expected = `start,end` of the overall
  match, then `;g1s,g1e;…`, or `-` for no match. Sections: literals,
  `.`, every bracket form (leading `]`, `^`, `-`, ranges, all 12 classes,
  negation, `[a-c[:upper:]]` — the bug in Goal 6 step 0), `*+?{m,n}`
  boundaries (`{0}`, `{0,0}`, `{255}`, `{256}` error), anchors in every
  context (`a^b`, `a$b`, `\(^a\)`, `^*`), alternation (`a|ab|abc`
  longest wins), back-references, `-i`, empty matches, NUL in subject,
  1 MB subject, `DFA_NOTBOL`, every error code once.
- **Differential test** (dev-only, `tests/dev/dfa-diff.c`, not CTest: it
  links the system `regcomp`). glibc supports `REG_STARTEND`, so subjects
  with NUL bytes are comparable. A random generator emits only POSIX
  tokens (no `\w`, `\b`, `\<`, `\|` in BRE), patterns of ≤ 12 tokens over
  an alphabet of 3 letters, and subjects of ≤ 10 characters over the same
  alphabet plus `\n`, so short strings hit ambiguity constantly. Compare:
  compile success (ignoring the documented differences: ERE `a{`, `a**`),
  overall span always, groups **only** when the pattern has no group under
  a repetition and no alternation of overlapping branches (else the
  documented sub-match deviation). Run 10⁶ cases per CI. Second oracle:
  the backtracker vs the DFA on the same pattern — spans must agree.
- **Compiler robustness.** Random *byte* strings as patterns under
  ASan+UBSan: must return an error or a program, never crash or hang;
  `((((…` × 100 000, `a{255}{255}`, `[[[[…`, trailing `\`, `\(\(…`.
- **Pathological time.** `(a*)*b`, `(a|aa)*b`, `(x+x+)+y`, `a?{25}a{25}`
  on 100 000 characters must finish in linear time in the DFA path;
  a backref pattern is allowed to be slow but must respect
  `DFA_MAXSTEPS`.
- **Builtins.** `tests/builtin-grep.sh`: one `assert_equal` per option
  and per output-format rule above, exit statuses (0/1/2, `-q` with an
  error), stdin, `-` operand, several files, a nonexistent file with
  and without `-s`, `-f` empty file, `-e` given twice, pattern with a
  leading `-`. `tests/builtin-sed.sh`: a table of `script|input|expected`
  from the spec's examples (the POSIX `sed` page's ones: `s/a/b/`,
  the `y` and `N;P;D` idioms, `$!N`, `1!G;h;$!d` = `tac`, `-n '$='` =
  `wc -l`), plus every rule marked "measured" above (`N` at EOF, missing
  final newline, `s/x*/-/g` empty-match rule).
- **Conformance suites already in the tree:** `tests/posix/*.tst` and
  `tests/yash/*.tst` run `expr`, `sed`, `grep` as *external commands* in
  places (they find the system's); check with `grep -l 'sed\|grep' tests/yash/*.tst`
  once the builtins exist — a builtin that shadows the system tool could
  change those results, in either direction, and that is worth knowing
  before shipping.

### Order of work (each step is its own change with test + `fixes/NN` + `BUGS`/`TODO.md` update)

1. **Import the `lib/` pieces** listed above (copy, don't rewrite);
   add `Makefile.in` for each new directory; a small `tests/` case for
   `buffer_getline_sa` on a line longer than the buffer.
2. **`lib/dfa/` parser + program + backtracker** behind
   `dfa_test`/`dfa_prefix`/`dfa_search`/`dfa_submatch` — already complete
   POSIX behaviour, slowest engine. `tests/dfa.sh`: a table of
   `(flags, pattern, subject, expected span/groups)` covering both
   syntaxes, every bracket form, intervals, backrefs, `*`-at-start,
   `^`/`$` context, `-i`, empty matches. Gate: ASan+UBSan build clean.
3. **Move `expr :` onto it** (and fix `BUGS: expr-crashes-and-exit-status`
   in the same pass, since the operator layer is being touched; each fix
   with its own `tests/fixed.sh` case). `tests/builtin-expr.sh` must stay green;
   measure the default-build size delta against the ≈+6 KB estimate.
   **Decision if it is worse:** keep the old matcher for builds without
   grep/sed, or accept it (record the number here either way).
4. **Lazy DFA** behind the same three entry points. Differential test
   (dev-only script, not CTest — needs the system tools): random
   patterns/subjects, DFA result vs backtracker vs libc `regcomp`;
   include `(a*)*b`, `(a|aa)*b`, empty-loop and 200 000-character
   subjects. The cache flush needs a test that forces it (tiny limit).
5. **`grep`** builtin (`EXTRA_BUILTINS`, off by default; `builtin.h`,
   `builtin_table.c` fallback define and entries, `cmake/Builtins.cmake`;
   autotools list in `configure.ac`). `tests/builtin-grep.sh`, skipped
   when not compiled in (same pattern as `tests/builtin-digest.sh`).
6. **`sed`** builtin, in this order so each stage is testable alone:
   parser + `p`/`d`/`q`/`=`/addresses; `s` with all flags (`g p w N i`, and
   `-E`); hold space
   commands (`g G h H x`); `n N D P`; `a i c r w y l`; labels and `b`/`t`.
   `tests/builtin-sed.sh`.
7. **Later, optional consumers** (only if wanted): see next section.

### What else could use it

POSIX utilities that take a BRE/ERE: `grep`, `sed`, `expr` (this goal),
`awk` (ERE; `~`, `split`, `sub`/`gsub`/`match` — needs exactly
`dfa_search` extents, but the language is a project of its own: busybox
`awk.c` is 4028 lines), `ed` (BRE; busybox `ed.c` 1030 lines), `ex`/`vi`,
`more` (`/re` search), `nl` (`-b pBRE`; busybox `nl.c` is 83 lines — the
cheapest add-on), `csplit` (BRE context lines), `pax -s` (BRE
substitutions). Not POSIX but common: `find -regex`, bash's `[[ =~ ]]`
(non-goal, see MAIN QUEST). Not useful: `case`/`${x#pat}`/globbing —
`path_fnmatch` is iterative and adequate, and a DFA would only add
size. `lex` *generates* DFAs, which is a different problem.

### Risks and open questions

- Backrefs make matching NP-hard: keep the DFA off patterns that have
  them and accept exponential worst case there (same as every POSIX
  engine).
- Group repetition in the backtracker recurses once per iteration; cap
  the depth or make single-atom repeats iterative so `\(a\)*` on a long
  line cannot exhaust the stack.
- Leftmost-longest is only guaranteed for the *overall* match; sub-match
  choice deviates from the strict POSIX rule in the same way glibc does
  (see "Sub-match rule — a known deviation"). musl/TRE is the stricter
  oracle if that ever matters.
- `\| \+ \?` in BRE (`DFA_GNU`): implementation-defined, so either choice
  conforms; accepting them makes GNU-written `grep`/`sed` scripts work,
  rejecting them makes shish's BRE strictly POSIX. Decide with the first
  real user; the flag costs ≈30 lines in the lexer.
- Compile time is paid per call: `sed` compiles once per script, `grep`
  once per run, but `expr` compiles on every invocation — fine, it is a
  one-shot builtin, and the lazy DFA builds no states until matched.
- `sed` "empty regex = last regex" is run-time state in the builtin, so a
  `struct dfa*` must stay alive across cycles; keep compiled regexes in
  the script's command array, not on the stack.
- `w /dev/stdout` and `r /dev/stdin` are special names in GNU only; POSIX
  does not define them. Support `/dev/stdout` (cheap, common in scripts),
  leave `r /dev/stdin` out.
- The line-length story: `buffer_getline_sa` streams any length; for a
  regular file `mmap_read` + `scan_lineskip` windows avoid the copy.
- Licence hygiene: nothing is copied from `ag.c`/`b.c`/`dfa1.c`; if that
  changes, check each licence against `COPYING` first.

### How to measure

```sh
# code size of the pieces (compare per object, stripped builds only)
cmake -S . -B /tmp/dfa -DCMAKE_BUILD_TYPE=MinSizeRel -DENABLE_GREP=ON -DENABLE_SED=ON
cmake --build /tmp/dfa -j8 && size /tmp/dfa/CMakeFiles/libowfat.dir/lib/dfa/*.o | sort -n
strip /tmp/dfa/shish && stat -c%s /tmp/dfa/shish     # vs the default build

# default-build cost of moving expr onto lib/dfa (step 3)
cmake -S . -B /tmp/def -DCMAKE_BUILD_TYPE=MinSizeRel && cmake --build /tmp/def -j8
```

---

## Goal 7 (secondary) — optional UTF-8 support (`WITH_UTF8`, off by default)

**Not started; this section is the plan.** shish is byte-oriented and
never calls `setlocale`, i.e. it is always in the POSIX locale, which is
what POSIX requires (Goal 6, Scope). This goal adds *character*
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
  utilities (Goal 6). **`printf` is not in this list**: POSIX.1-2024
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
| C | pathname expansion | `expand_glob.c` calls the **libc** `glob()` | policy decision (below) |
| D | IFS with non-ASCII characters | `expand_cat.c`, `expand_glob.c`, `builtin_read.c` | small, rare case |
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

`S_STRLEN`: `expand_cat(lstr, fmt_ulong(lstr, mb_len(v, vlen)), …)`.
`S_RANGE` (`${v:off:len}`, characters like bash): `nc = mb_len(v, vlen);
r = limit(&r, 0, nc); o = mb_off(v, vlen, r.offset);
e = o + mb_off(v + o, vlen - o, r.length);` then `expand_cat(v + o, e - o, …)`.
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

`builtin_read.c` / `expand_cat.c` / `expand_glob.c` (IFS, M6): today
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
| `expand_param.c:86-90`, `expand_glob.c:79-81` | `"$*"` joins with `ifs[0]` | `mb_clen(ifs, ifslen)` bytes | 4 |
| `lib/path/path_fnmatch.c` | `?`, `*` backtrack, bracket members/ranges by byte | `?` and star step by `mb_clen`; bracket members decoded with `mb_get` (`[éü]`, `[à-ÿ]` by code point); literal runs unchanged (byte compare is correct) | 40-60 |
| `expand/expand_glob.c` | libc `glob()` | see "libc" below | 5-10 |
| `expand_cat.c:10-16`, `expand_glob.c:42`, `builtin_read.c:122-141` | IFS as a *byte* set (`str_chr`, `scan_charsetnskip`) | only matters when IFS contains non-ASCII: add `mb_span`/`mb_cspan`. **P3**, known gap until done | 30 |
| `builtin_printf.c:308-345` | `%s` width/precision and `%c` in bytes | **none** — POSIX says bytes (see above) | 0 |
| `builtin_wc.c` | `-m` = bytes; `-L` = bytes | `-m`: `mb_len` (one per character; invalid byte = 1); `-L`: `mb_cols` | 20 |
| `builtin_expr.c`, `lib/dfa` | bytes | `length`/`index`; regex via `DFA_UTF8` (Goal 6) | later |
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
(`expand_glob.c:5-19`; `lib/unix/glob.c` is Windows-only), and glibc/musl
`glob`/`fnmatch` follow the locale set with `setlocale`, which shish
never calls. Options: (a) leave it — `?` in a *pathname* pattern stays
bytewise, everything else is right (documented gap); (b) under
`HAVE_SETLOCALE`, call `setlocale(LC_CTYPE, "")` when `mb_utf8` turns on
so libc agrees (glibc, musl); dietlibc/mingw stay at (a). Start with (a),
add (b) as its own step. The internal `glob` planned in Goal 5.3
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
(Goal 5's tooling) and treat any growth beyond a few bytes of noise as a
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
- **M7** — Goal 6's `DFA_UTF8`: character sets as code-point ranges,
  compiled to UTF-8 byte-sequence automata so the DFA stays byte-based
  (the 256-bit bracket sets in Goal 6 cover the ASCII part only).

### Risks and open questions

- Character classes for non-ASCII stay ASCII-only in the first pass;
  say so in `--help`/docs, or it will be reported as a bug.
- `LC_ALL=C.UTF-8` as a command-prefix assignment: see the hook risk above.
- `${v:off:len}` is a non-POSIX extension already on by default
  (`WITH_PARAM_RANGE`); its UTF-8 behaviour follows bash (characters).
- Invalid input: nothing may crash or loop — fuzz the `mb_*` functions
  with random bytes under ASan+UBSan (MAIN QUEST gate) before M2 lands.
- `printf` `%c`/`%.Ns` stay bytes (POSIX.1-2024); a later "bash
  compatibility" request to make them characters is a scope change, not a bug.
- `"$*"` with an empty IFS is broken independently of UTF-8
  (`BUGS: star-empty-ifs-appends-nul`); fix it before touching that
  line, or M2's `mb_clen(ifs, …)` would return 1 for the NUL terminator.

---

## Goal 8 (secondary) — `shfuzz`: a structure-aware shell fuzzer built on `union node`

**Not started; this section is the plan.** Generate random but *valid*
ASTs (`src/tree.h`) from entropy or a key, serialize them with
`tree_cat()`, and feed the text to the parser/evaluator. Useful as a
test (round-trip and sanitizer oracles) and as a standalone utility.
Dev tool: built only with a `BUILD_SHFUZZ` option, **never linked into
`shish`** (the constraint table alone would cost several KB).

### What `tree_cat.c` shows (checked 2026-09-19 by grep and by running `shformat`)

`tree_cat()` is the ground truth for which parts of `union node` matter
to a serializer — but it is lossy, and the loss is exactly the set of
members it never reads:

| Read by `tree_cat` | Never read |
|---|---|
| child slots (`args vars rdir cmds pats word list test cmd0 cmd1 body left right node cond ontrue onfalse tree`), `nargstr.flag & S_TABLE`, `nargparam.flag` (`S_STRLEN S_VAR S_SPECIAL S_ARITH S_NULL`, `S_RANGE` under `WITH_PARAM_RANGE`) + `numb` + `name`, `nargcmd.flag & S_BQUOTE`, `nredir.flag` (`R_IN R_OUT R_APPEND R_DUP R_HERE R_STRIP R_CLOBBER`) + `fdes`, `narithnum.base` ∈ {8,10,16} + `num`, arithmetic ids, `nlist.bgnd` | `ncmd.bgnd` and every other `bgnd`, `nfor.has_in`, `nredir.data`, `nredir.fd`, `npipe.ncmd`, all `loc`, `narg.flag` (the `X_*` expansion bits) |

Three real bugs fall out of the right-hand column (`BUGS:
tree-cat-drops-background-ampersand`, `tree-cat-drops-empty-for-in`,
`tree-cat-mangles-here-documents`; e.g. `for x in; do …` becomes
`for x; do …`, which changes the program). A generator restricted to
what `tree_cat` reads would never exercise `bgnd`, `has_in` or here-doc
data, although `eval` uses all three — so fix `tree_cat` first, or the
round-trip oracle fails on every tree that touches them.

### The constraint table: `tree_spec[]`, one row per `enum kind`

`enum kind` has 63 values (`N_SIMPLECMD` … `A_VBITOR`), and
`tree_nodesizes[]` (`src/tree/tree_nodesizes.c`) is already a per-kind
table, so the shape is established. Use C99 designated initializers
(`[N_REDIR] = {…}`) — `tree_nodesizes[]` is positional and would silently
shift if the enum were reordered. A set of allowed kinds fits a
`uint64_t` mask (bit `1ull << kind`, one spare bit).

```c
struct tree_member {               /* one *slot* of one node struct */
  uint16_t offset;                 /* offsetof(struct nredir, flag) */
  uint8_t  role;                   /* ONE OPT LIST | INT FLAGS ENUM | STR | COUNT */
  uint8_t  sep;                    /* LIST: what ->next means here (';' '|' ' ' ',' ...) */
  uint16_t min, max;               /* LIST length */
  uint64_t kinds;                  /* child roles: allowed kinds, bit per enum kind */
  int64_t  lo, hi;                 /* INT/ENUM: inclusive bounds (named TREE_MAX_* constants) */
  const struct tree_group* groups; /* FLAGS: independent sub-fields, e.g. S_TABLE = {0, S_DQUOTED, S_SQUOTED} */
  uint8_t  pred;                   /* STR: TS_NAME TS_SQ TS_DQ TS_WORD ... (see below) */
};
struct tree_spec {
  uint16_t size;                   /* == tree_nodesizes[kind]; static assert */
  uint8_t  nmember, min_depth;     /* min_depth: levels needed to finish, so generation terminates */
  struct tree_member m[TREE_MAXM];
  void   (*fixup)(union node*);    /* rare cross-member rules, see below */
};
const struct tree_spec tree_spec[A_VBITOR + 1] = {
  [N_SIMPLECMD] = {…, { LIST(ncmd, args, ' ', 0, 8, K(N_ARG)),
                        LIST(ncmd, vars, ' ', 0, 4, K(N_ASSIGN)),
                        LIST(ncmd, rdir, ' ', 0, 3, K(N_REDIR)) } },
  [N_REDIR]     = {…, { FLAGS(nredir, flag, R_GROUPS), INT(nredir, fdes, 0, TREE_MAX_FD),
                        LIST(nredir, word, '\0', 1, 1, K(N_ARG)) } },
  …
};
```

**`next` is not a member of the table.** Every one of the 22 `n*` structs
has it (common prefix `id`, a 1-bit field, `next`), so "has `next`" says
nothing. Whether a node sits in a chain is decided by the *slot that
points at it*:

| Role | Meaning | Validator rule |
|---|---|---|
| `ONE` | exactly one node | `->next == NULL` |
| `OPT` | zero or one | `NULL`, or a node with `->next == NULL` |
| `LIST` | chain, `min`..`max` long, separator tag `sep` | each element's kind ∈ `kinds`; last `next` is `NULL` |

The tree root is a pseudo-slot of role `LIST` (the top-level command
sequence) and is the generator's entry point. `sep` records that `next`
means `;` in a compound list, `|` in `npipe.cmds`, a space in
`ncmd.args`, `|` between case patterns, `,` in `A_PAREN`.

Initial slot roles, read from `tree_cat.c` (to be confirmed by method C):

| Slots | Serialized with | Role |
|---|---|---|
| `ncmd.args/vars/rdir`, `nfor.args/cmds`, `nloop.test/cmds`, `ngrp.cmds`, `nlist.cmds`, `ncase.list` (of `N_CASENODE`), `ncasenode.pats/cmds`, `nif.test/cmd0/cmd1`, `nargcmd.list`, `nargarith.tree`, `A_PAREN.tree`, `nredir.word` | `tree_catlist` / manual loop | `LIST` |
| `npipe.cmds`, `narg.list` (of `N_ARGSTR/ARGPARAM/ARGCMD/ARGARITH`) | manual loop | `LIST` |
| `ncase.word`, `nargparam.word`, `nfunc.body`, `A_TERNARY` slots, binary `left`/`right`, unary `node` | `tree_cat` | `ONE` |
| `nandor.left/right`, `nnot` | `tree_catlist` | **unclear** — the parser probably builds a single node; a chain given to a `tree_cat`-single slot silently loses its tail |

Rules that are not per-slot:

- The 1-bit field is called `bgnd` in 9 structs (`ncmd npipe nandor ngrp
  nfor ncase nif nloop nlist`), `dummy` in 7 (`nnot ncasenode nfunc
  narithnum/unary/binary/ternary`), and the 6 word-level structs (`narg
  nredir nargstr nargparam nargcmd nargarith`) have a full `unsigned flag`
  instead; the validator requires `dummy` bits to be 0.
- `nif.cmd1` holding exactly one `N_IF` prints as `elif`; `else if … fi`
  parses to the same shape, so both are the same tree.
- `nfunc.next` is typed `struct nfunc*`; treated as the same link.
- Whole-tree invariants: acyclic, and no node reachable twice
  (`tree_free` would double-free).
- Rare cross-member rules go in the per-kind `fixup` hook, only for:
  `npipe.ncmd == length(cmds)`; `nargparam` uses `numb` when
  `S_SPECIAL == S_ARG`, else `name`; an `R_HERE` redirection needs `data`.
- Strings are constrained by **predicates that already exist**
  (`parse_isname`, `parse_isesc`, `parse_isdesc`, `var_valid`) — the
  table stores a predicate id, the generator calls the predicate.
  Examples: a `'…'` string cannot contain `'`; `nfor.varn`, `nfunc.name`
  and `nargparam.name` must be valid names and not reserved words.
- Integers need **named, asserted bounds** (`TREE_MAX_FD` for
  `nredir.fdes`, one for `numb`, `num`/`base`), enforced in the parser
  as well; today they are plain `int`/`long`/`int64` and it has not been
  checked what `parse`/`eval` really tolerate.
- Not in the table: evaluation safety (the fuzzer must not generate
  `rm -rf`); that is generator policy, see below.

The same table drives `tree_validate(node)` (first out-of-spec member,
with a path). Uses: the generator's postcondition; a debug-build check
after every `parse()` — which enforces "integers never exceed what
parse/eval/expand expect" and finds either a wrong table or a parser
emitting out-of-spec trees. Later, `tree_free`/`tree_copy`/a
`tree_equal` could be made table-driven; not part of this goal.

### How the table gets filled: three derivations, used together

- **A. Static extraction from `tree_cat.c`.** Parse the file with clang
  (`-ast-dump=json` or `clang-query`); per `case` label record member
  accesses, constants in `& S_…`/`& R_…` tests, `switch` labels (the `base`
  cases), null checks (⇒ optional), `for(… n = n->next)` loops and
  `tree_catlist` vs `tree_cat` calls (⇒ `LIST` vs `ONE`), array sizes as
  domains (`vsubst_types[(flag & S_VAR) >> 8]` ⇒ 0..7). Emit an X-macro
  header. Mechanical and regenerable; needs clang (dev-only). Traps:
  `goto again`, the `N_NOT → N_AND` fallthrough, `#if WITH_PARAM_RANGE`,
  and accesses through an aliasing member (`node->ncmd.rdir` on `N_IF`
  relies on identical offsets — emit `offsetof` equality checks).
  Decides **which bits matter**.
- **B. Black-box sensitivity probing.** Compile against the real structs;
  per kind build a canonical valid node, flip every scalar bit and see
  whether the `tree_cat` output changes; sweep values for crashes; try
  every kind in every pointer slot for the allowed-child matrix
  (≈63 × 4 probes); cross-check against `tree_free`/`tree_copy`. Needs no
  C parsing and tests real behaviour, but cannot tell a domain violation
  from a bug the parser can reach, and is too permissive where `tree_cat`
  tolerates what `eval` would not. Used as a **CI check** that the table
  still matches reality.
- **C. Parser-observed profile.** A small tree walker run over every
  script we have (`tests/*.sh`, `tests/posix/*.tst`, `tests/yash`);
  record per (parent kind, slot) the child kinds, chain lengths and the
  observed value ranges of scalars. Stays within what `parse`/`eval`/
  `expand` handle in practice — the stated requirement — but only as
  complete as the corpus. Decides **domains and bounds** and settles the
  unclear `ONE`/`LIST` slots.

### What the generator is (definition, 2026-09-19)

A deterministic function `decode(bits) → AST`, i.e. a *structure-aware
test-case generator* with a fuzzing driver on top (prior art: Hypothesis'
choice sequences and Zest/JQF generators-as-parsers-of-bytes, LLVM's
`FuzzedDataProvider`, `libprotobuf-mutator`; for enumeration SmallCheck
and Korat). Required properties:

- **Total:** every bitstring decodes to a valid AST, nothing is rejected.
  Budgets and "default choice when the bits run out" deliver this.
- **Deterministic:** same bits → same AST → same script text. This is a
  property of *generation only*: executing the script is not
  deterministic (time, pids, `$RANDOM`, jobs, signals), so "same key,
  same behaviour" does **not** hold in evaluate mode.
- **Surjective** onto the valid ASTs within the budgets.
- **Many-to-one, and that is fine:** trailing unused bits, leading zeros
  and default choices collapse. "Every bit gives a different AST" is not
  true of a stream decoder.

Two designs share the same `tree_spec[]`; build the first:

| | Stream decoder (**first**) | Enumerative unranking (later) |
|---|---|---|
| Mapping | bits → AST, many-to-one | integer *n* → the *n*-th AST by size, bijective |
| For | random and coverage-guided fuzzing, mutation, **shrinking** a failing case by shortening/simplifying the stream | exhaustive small scope: "no crash for every AST up to size *k* over this word pool" |
| Weak | no coverage guarantee | no bit locality, so it does not mutate well |

"Counting 1, 2, 3, … generates every script" is only true of the
bijective design, and only in principle: binary counting changes the low
bits fastest, so a stream decoder fed 1, 2, 3, … varies only its last
~20 choices (early structure stays on defaults), and the space explodes
(8 words, commands of 1-3 words, scripts of 1-3 commands is already
≈2·10⁸ scripts before any nesting). Enumeration is only useful at small
size, as a guarantee, not as a way to find bugs in big scripts.

**ASTs are a quotient of scripts, not all scripts.** Whitespace,
comments, quoting variants and line continuations collapse into one AST,
and scripts that fail to parse are not ASTs. So valid-AST generation
never reaches the lexer's odd paths or the parser's rejection and
error-recovery paths — where memory-safety bugs often live — and `tree_cat`
emits one canonical style, so the round trip only tests that slice.
Two additions, both in scope:

- a **second serializer with randomized style** (extra whitespace,
  alternative-but-equivalent quoting, comments, `\`-newline
  continuations); the round-trip oracle still holds because the tree must
  not change;
- shfuzz output as **seed corpus** for byte-level fuzzing (AFL/libFuzzer)
  of the parser, which covers the invalid-input side.

### Words (strings) are synthesized, not enumerated

Strings dominate the search space (unbounded length × 256 symbols) and
the parser treats characters by class, so collapse them — deterministically,
from the same stream:

- **Role-typed pools:** name role → a small pool or `[a-z_][a-z0-9_]{0,k}`;
  numbers `0 1 -1 …`; glob patterns `* a* [a-c] ?`; fds; here-doc
  delimiters. The role comes from the table's string predicate.
- **Reduced alphabet:** one representative per character class the lexer
  and expander distinguish (letter, digit, `_`, space, tab, newline, `/`,
  `.`, `-`, `=`, `$`, `\`, `'`, `"`, `` ` ``, `*`, `?`, `[`, `]`, `!`, `~`,
  `#`, `%`, `{`, `}`, `|`, `&`, `;`, a non-ASCII byte).
- **Escape hatch:** one choice bit occasionally draws raw bytes, so rare
  bytes are not lost entirely.
- **Typed symbol pools:** variables, functions and files created earlier
  in the generated script are reused by later words; evaluation coverage
  is much better than with random names. This makes the generator
  slightly context-sensitive, kept as a small side table.

### Generator mechanics

- Input is a **byte stream**: every choice takes bits from it (`take(n)`);
  when it runs out, choices default to the minimal option. A key seeds a
  PRNG (splitmix64) that expands into that stream, so "from entropy or a
  key" is the same code, and libFuzzer/AFL can drive it with their own
  mutation of the bytes.
- Budgets: max depth, max list length per slot (`max` in the table),
  max string length, total node count; `min_depth` guarantees every
  recursion can be closed.
- Two modes: **parse-only** (safe by construction — *the first release*)
  and **evaluate**, which is a project of its own: word/command
  whitelist, scratch directory, `ulimit`s, timeouts for generated
  `while true` loops, no `/dev`-touching redirections, and normalisation
  of nondeterminism before any comparison. Evaluate mode stays **out of
  the first release**.

### Oracles

1. **Round trip:** `tree_cat(T)` → `parse` → `tree_cat` must reproduce
   the string, and the trees must match under a new `tree_equal` that
   ignores `loc`. Deviations are serializer or parser bugs (`tree_cat`
   fixes first, see above).
2. `tree_validate` on every parser output (debug builds).
3. ASan+UBSan build must run clean (MAIN QUEST gate); crash-only mode
   for long runs.
4. Optional differential run of the evaluated result against `dash`/
   `bash`/`yash` in a sandbox, for the POSIX-only subset the generator
   emits (evaluate mode only; noisy — bugs and bash-isms on both sides).

Without a reference shell the oracles find crashes, hangs, sanitizer
errors and round-trip mismatches, **not wrong behaviour**; say so in the
tool's documentation.

### Order of work (each step its own change with test and this file updated)

0. Fix the three `tree-cat-*` bugs in `BUGS` (regression cases in
   `tests/fixed.sh`, `fixes/NN`), or the round trip cannot be the oracle.
1. `TREE_MAX_*` constants and their parser-side asserts; write the tree
   walker (also needed by C) and `tree_equal`.
2. Method C profile over the corpus → first hand-written `tree_spec[]`
   for a subset (simple commands, pipelines, lists, words).
3. `tree_validate` + debug-build hook after `parse()`; every violation
   found is either a table fix or a `BUGS` entry.
4. Stream-decoder generator for that subset with word synthesis,
   round-trip test in `tests/`, ASan run. **This is the MVP and the
   go/no-go point:** ship `shfuzz` as a sibling of `shparse2ast`
   (`BUILD_SHFUZZ`, off by default, not installed, not linked into
   `shish`) only once it has found bugs.
5. Methods A and B: regenerate/verify the table; extend to compound
   commands, redirections, parameter expansion, arithmetic.
6. Randomized-style serializer (lexical noise) and shfuzz output as a
   seed corpus for AFL/libFuzzer on the parser; shrinking by stream
   reduction; standalone command line (`shfuzz KEY`, `shfuzz -n COUNT`).
7. Later, separately: enumerative unranking for small-scope exhaustive
   runs; evaluate mode with its sandbox.

### Size estimate (estimates, nothing written; calibration: `tree_free.c`
213 lines and `tree_copy.c` 194 lines are per-kind switches, `tree_cat.c` 613)

| Piece | Lines |
|---|---|
| `tree_spec[]` (63 rows, macros for the shared layouts) | ≈250-350 |
| `tree_validate` | ≈100-150 |
| `tree_equal` + walker | ≈150-200 |
| generator (byte-stream decoder, budgets, predicates, word synthesis) | ≈400-600 |
| randomized-style serializer (step 6) | ≈150-250 |
| method A extractor (clang script) / B prober / C profiler | ≈150 / ≈200 / ≈120 |
| harness + CLI + tests | ≈300 |

### Risks and open questions

- Parse-time state that the AST does not carry: aliases, `loc`/`$LINENO`,
  reserved-word context — the round trip must run with aliases off and
  compare without `loc`.
- Quoting is the hard part of the string domains: `nargstr` content
  depends on the enclosing quote state (`parse_isesc`/`parse_isdesc`); a
  wrong predicate makes every round trip fail for the wrong reason.
- Keep the table dev-only; if a debug-build `tree_validate` hook is
  wanted in `shish`, it must stay behind `DEBUG_*` like the other
  instrumentation.
- Method C is bounded by the corpus; legal-but-unseen combinations need
  the grammar in POSIX 2.10 as a cross-check.
- **Maintenance coupling:** every change to `tree.h` touches the table;
  static asserts (`sizeof`, `offsetof`, row count) and method A's
  regeneration reduce this but do not remove it. If the tool rots, delete
  it rather than let a stale table produce wrong reports.
- Word synthesis collapses the string space by design; the raw-byte
  escape hatch keeps some weird-byte coverage, but string-level bugs
  (glob, `printf` formats, IFS) still want their own byte-level fuzzing.
- Value: mainly for shell implementers and for CI (reproducible key →
  bug report, "no crash for all ASTs ≤ *k*"); end users get little, hence
  a build option and no installation by default.

---

## Goal 9 (secondary) — tab-completion: context-aware, and extensible through `complete`

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
  `mb_cols` once Goal 7 exists.
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
  random walk: generate a valid shell fragment (the Goal 8 fuzzer, when it
  exists), cut it at a random word boundary, and assert that the
  continuation's next keyword is among the offers.
- **`complete`** (Phase 5): every option, `-p` round trip
  (`complete -p | eval` reproduces the registry), `-F` with `COMP_*`,
  runaway function aborted by the guard, `$?` preserved, a `-F` that
  writes to stderr leaves the line intact (pty case), subshell isolation.
- **Editor glue** (pty, few cases): insertion, redraw after a list, cursor
  in the middle of a line (text after the cursor is kept and not
  considered), two-TAB list behaviour.
- **Fuzz**: random bytes as the line and a random cursor position under
  ASan+UBSan (Goal-8 style gate): `compl_words()` and `compl_context()`
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
stripped-size comparison from Goal 5.

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
  `EXTRA_BUILTINS`? Decide from the measured size (≈6 KB, Goal 5.5 policy).
- **Correctness of the lexer vs. the parser**: Phase 4 accepts small drift;
  the fallback keeps it harmless. Track drift cases in `BUGS` as they are found.
- **`$PATH` scan cost** on slow/network directories: the cache is per
  directory mtime, but the first scan blocks the prompt; a soft limit
  (`COMPLETION_PATH_MAX`, default 5000 entries) and no scan of directories
  that are not `stat`-able within one call.
- **UTF-8** (Goal 7): candidate width in the listing and the insertion of a
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

## Goal 10 (secondary) — `cp` and `mv` builtins: one source file, dispatch on `argv[0]`

**Not started; this section is the plan.** Neither exists today (only
`ln`, `link`, `rm`, `rmdir`, `mkdir`, `touch`, `cat`, `tee`, `ls`, …,
all in `EXTRA_BUILTINS`). Both go into `EXTRA_BUILTINS` too: they are useful
where no external `cp`/`mv` exists — the WASI build (`doc/wasm.md`: no
`fork`, so no external commands at all), containers and single-binary
images — and cost nothing where they are off.

### Would one source file with `argv[0]` dispatch save size?

**Yes, modestly; the reason is sharing code, not table rows.** Measured
2026-09-20 on this tree (`-DENABLE_ALL_BUILTINS=ON`, object `text+data`):

| Builtin | Source lines | Object bytes |
|---|---|---|
| `link` | 33 | 301 |
| `mkdir` | 105 | 739 |
| `rmdir` | — | 802 |
| `cat` | 116 | 928 |
| `ln` | 106 | 933 |
| `rm` | 157 | 1249 |
| `ls` | 274 | 2108 |
| `touch` | 326 | 3413 |

≈ 7-10 bytes of object code per source line (same calibration as Goal 6).

- **What `mv` is:** `rename()`, and when that fails with `EXDEV`, *copy the
  hierarchy (`cp -R -p` semantics, symlinks copied as links), then remove
  the source (`rm -r`)*. So `mv` needs everything `cp` has, plus `rm`'s
  recursive delete. Two separate source files would either duplicate the
  copy engine or export it; either way they share ≈ 80 % of their code.
- **Table rows cost the same in both layouts** (two rows: `cp`, `mv`;
  ≈ 40 bytes each, plus 4 relocations per row in a PIE build). There is
  nothing to save there.
- **`--gc-sections` is on** (`-ffunction-sections`, `cmake/Checks.cmake`), so
  a function nobody references is dropped even from a shared object file:
  merging does **not** force `cp`-only code into a `mv`-only build, or the
  reverse.
- **What merging saves** is the duplicated glue: option loop, "last operand
  is the target, is it a directory?" logic, `basename` join, the `-i`
  prompt, error/verbose reporting, plus one object's alignment padding.
  Estimate **≈ 60-90 source lines ≈ 0.5-0.9 KB** out of ≈ 4-5 KB for the
  pair (**≈ 12-18 %**). A single function that dispatches on `argv[0]`
  (one getopt string with per-mode masks) saves a further ≈ 150-250 B over
  two exported functions that share static helpers, at the price of `mv`-only
  builds carrying `cp`'s option parsing (`-R -H -L -P -p`, ≈ 100-200 B) —
  which `mv`'s `EXDEV` path uses anyway.
- **These are estimates.** Step 3 of the order of work builds both layouts
  and keeps the smaller (`size` on the stripped `MinSizeRel` build, and the
  `nm --size-sort` of the object); the plan below assumes the dispatch
  version, which is the cheaper one to turn into two entry points if the
  numbers disagree.

**Decision:** one file, `src/builtin/builtin_cpmv.c`, one exported function
`builtin_cpmv(argc, argv)`; two table rows pointing at it, with **separate
help strings** (`help_cp`, `help_mv`), same pattern as
`builtin_digest.c` (`md5sum`…`sha512sum`, `builtin_table.c:313`). It is
compiled into `libshell` always and pulled in by the linker only when a row
references it (`BUILTIN_CP`/`BUILTIN_MV`, default 0 in the
`builtin_table.c` fallback defines). `argv[0]` is the name the builtin was
called by (`cp`, `mv`; `command cp` and `builtin cp` keep it); an
unrecognised name is an internal error (`unknown name`), as in `digest`.

### Scope and options

POSIX.1-2024 (read 2026-09-20):

| | Options | Forms |
|---|---|---|
| `cp` | `-P -f -i -p`; `-R` with `-H -L -P` (last wins) | `cp [-Pfip] src target_file` · `cp [-Pfip] src... target_dir` · `cp -R [-H\|-L\|-P] [-fip] src... target_dir` |
| `mv` | `-f -i` (last wins) | `mv [-if] src target_file` · `mv [-if] src... target_dir` |

Decided extensions (each is a few lines; all are accepted by GNU, BSD and
busybox, and by every script that matters):

| Option | For | Meaning | Cost |
|---|---|---|---|
| `-r` | `cp` | same as `-R` (POSIX marks it obsolescent-but-required-by-history) | 0, one `case` label |
| `-v` | both | print `'src' -> 'dst'` (same style as `ln -v`, `rm -v`) | ≈ 6 lines |
| `-n` | both | never overwrite an existing destination; not an error | ≈ 4 lines |
| `-a` | `cp` | `-R -P -p` (archive) | ≈ 2 lines |
| `-T`, `-t DIR` | both | no-target-directory / target directory first | ≈ 15 lines each — **not in the first pass** |
| `-u`, `-l`, `-s`, `--reflink`, `--preserve=…`, `-b`, `--backup` | | GNU-only | not planned |

Long options: none (like the other builtins). `--` ends options
(`shell_getopt`).

### `cp` — behaviour, step by step (POSIX steps, with the syscall each maps to)

Per `source_file`, after resolving `dest_file` (`target/basename(src)` when
`target` is an existing directory; `basename` on a copy of the string —
`builtin_ln.c` calls `basename()` on `argv` in place, which may modify it):

1. **Same file** (`st_dev`/`st_ino` equal after `stat`): diagnostic
   `cp: 'a' and 'b' are the same file`, no copy, status 1. Applies to
   `cp a a`, hard links, and `cp a dir/` when `dir/a` is `a`.
2. **Directory source**: without `-R` → diagnostic `cp: -r not specified;
   omitting directory 'x'`, skip, status 1. With `-R`: create the
   destination directory with the source's mode **& ~umask, | S_IRWXU** while
   copying (so files can be created inside), recurse, then `chmod` to the
   source's final mode (POSIX step 2); never follow into `.`/`..`. If `dest`
   exists and is not a directory → diagnostic, skip subtree.
   **Copy into itself** (`cp -R a a/b`): detect by walking `dest`'s ancestors
   comparing `(st_dev, st_ino)` with the source directory; diagnostic, skip.
3. **Regular file**, destination exists: with `-i` prompt on stderr
   (`cp: overwrite 'dst'? `), read one line from **stdin**, continue only on
   `y`/`Y` at the start of the reply (locale `yesexpr` is not consulted:
   only `y`/`Y`); `-n` skips silently. `open(dst, O_WRONLY|O_TRUNC)`;
   if that fails and `-f`: `unlink(dst)` then `open(dst, O_WRONLY|O_CREAT|O_EXCL)`.
   Destination missing: `open(dst, O_WRONLY|O_CREAT|O_EXCL, srcmode)` (the
   kernel applies the umask). Copy loop: `read`/`write` in **64 KiB** blocks
   from a heap buffer (not `.bss`, Goal 5.6; not the stack), handling short
   writes and `EINTR`; on Linux `copy_file_range()` is an optional fast
   path behind a `HAVE_COPY_FILE_RANGE` check, **not in the first pass**.
   Holes are not preserved (zeros are written).
4. **Special files under `-R`**: FIFO → `mkfifo`, symlink under `-P` (or
   default) → `readlink` + `symlink`, device nodes → diagnostic
   "cannot copy special file" (no `mknod`; POSIX leaves it implementation-
   defined). Sockets → diagnostic.
5. **`-p`**: after the data is written: `utime()` (access and modification
   time; the same call `touch` uses, portable to MinGW), `chown()`
   (failure is silently tolerated when not root, **but then clear
   `S_ISUID|S_ISGID`** as POSIX requires), `chmod()` to the source mode.
   Without `-p`: the mode is the source mode masked by the umask, and
   set-uid/set-gid bits are not kept.
6. **Symlink operands and traversal**: without `-R`, follow (unless `-P`);
   with `-R` and no `-H/-L/-P`, **`-P`** (POSIX: unspecified; the safe choice,
   and what GNU/BSD `cp -R` effectively do for traversal). `-H`: follow only
   command-line symlinks; `-L`: follow all.
7. **Exit status**: 0 if everything requested was copied (a declined `-i`
   prompt is not an error); 1 otherwise; the loop continues to the next
   source after an error. Diagnostics use `builtin_error()`
   (`cp: path: strerror`).

Hard links inside a hierarchy are copied as separate files (POSIX:
unspecified; GNU preserves them only with `-d`/`-a`; not planned).

### `mv` — behaviour, step by step

1. **Prompt** (POSIX step 1): destination exists, no `-f`, and either it is
   not writable (`access(dst, W_OK)`) **and stdin is a terminal**, or `-i`
   was given → prompt `mv: overwrite 'dst'? `, proceed only on `y`/`Y`.
   `-n`: skip silently.
2. **Same file** (`st_dev`/`st_ino`): diagnostic `mv: 'a' and 'b' are the
   same file`, nothing removed, status 1 (POSIX allows three outcomes; this
   is the safe one, and the same as GNU).
3. **`rename(src, dst)`**. Success → done for this source. (On
   `WINDOWS_NATIVE`, `rename` fails when the destination exists: use
   `MoveFileExA(…, MOVEFILE_REPLACE_EXISTING)` there.)
4. **Type mismatch** (dir over non-dir or non-dir over dir): diagnostic,
   skip; do not fall through to copy. (`rename()` itself returns
   `EISDIR`/`ENOTDIR`/`ENOTEMPTY`, which are reported as
   `mv: cannot overwrite 'dst': …`.)
5. **`EXDEV`** (other file system) is the only error that triggers the
   fallback: `cp -R -p`-style copy into a **temporary sibling name**
   (`dst.mv.NNNN`, `O_EXCL`), `rename` the copy to `dst` when complete,
   then remove the source hierarchy with the shared `rm` tree removal.
   Ordering matters for data safety: **the source is removed only after the
   whole copy succeeded**; on any error the temporary is removed and the
   source is left untouched.
6. **Characteristics** (POSIX step 6): times, uid/gid and mode are copied;
   if uid/gid/mode cannot be duplicated, set-uid/set-gid are dropped and a
   diagnostic is written **without changing the exit status** (POSIX quote).
   Symlinks are moved as symlinks. Hard links across file systems are
   copied as separate files (unspecified in POSIX).
7. **Exit status**: 0 if all moved (a declined prompt is not an error), 1
   otherwise; continue with the next source after an error.
8. `mv a a` and moving a directory into itself (`mv d d/sub`) are detected
   before step 3 by the same ancestor walk as `cp`; diagnostic, no action
   (`rename()` would return `EINVAL` anyway; the message is what improves).

### Shared internals (all `static` in `builtin_cpmv.c`, except the last)

| Helper | Job | Lines (est.) |
|---|---|---|
| `cpmv_parse()` | one option loop; `argv[0]` picks the option string (`"RrHLPfipnva"` for `cp`, `"finv"` for `mv`) and the masks that reject the other's flags | 45 |
| `cpmv_target()` | last operand + `stat`: directory or not; "more than two operands need a directory" error | 30 |
| `cpmv_dest()` | join `target/basename(src)` in a `stralloc` (basename of a copy) | 15 |
| `cpmv_same()` / `cpmv_inside()` | `(dev,ino)` identity and ancestor walk | 35 |
| `cpmv_confirm()` | prompt on `fd_err`, read a line from `fd_in` (see "Interactive prompt") | 30 |
| `cpmv_copy_data()` | the read/write loop | 30 |
| `cpmv_copy_attrs()` | `-p` handling and set-id clearing | 30 |
| `cpmv_copy_tree()` | recursion: dir / file / symlink / fifo | 80 |
| `cpmv_cp_one()`, `cpmv_mv_one()` | the per-source step lists above | 50 + 40 |
| `builtin_cpmv()` | dispatch + operand loop + status | 40 |
| **`builtin_rm_tree()`** (exported from `builtin_rm.c`) | the existing static `rm_recursive()`, renamed and declared in `builtin.h`, so `mv` removes a source hierarchy with the code `rm -r` already has | 0 new (rename only) |

Estimated total **≈ 425 lines ≈ 3.5-4.2 KB** for both, versus **≈ 490-520
lines ≈ 4.2-5 KB** if `mv` had its own copy of the glue.

### Portability

- **Feature checks** (`cmake/Checks.cmake`, next to `HAVE_LSTAT`/
  `HAVE_READLINK`): `HAVE_UTIME`, `HAVE_CHOWN`, `HAVE_MKFIFO`,
  `HAVE_SYMLINK`, `HAVE_RENAME` (always), optional `HAVE_COPY_FILE_RANGE`.
  Every attribute step is `#if`-guarded and degrades to a no-op.
- **`WINDOWS_NATIVE`** (mingw builds exist): `lstat`/`readlink`/`symlink`
  shims are in `lib/unix/`; no `chown`/`mkfifo`; `rename` needs the
  replace-existing wrapper; mode bits are the emulated ones.
- **WASI** (`doc/wasm.md`): `rename`, `open`, `read`/`write`, `readdir`,
  `utime` exist; `chown`, `mkfifo`, `symlink`, `chmod` do not (wasi-libc) —
  the `HAVE_*` guards cover them; `EXDEV` cannot occur between preopened
  directories in practice, so the fallback is compiled but rarely runs.
  `cp`/`mv` are the point of this port: they are the first file utilities
  that work with no `fork`.
- **dietlibc/musl**: only standard calls; `utime` not `utimensat`.
- **Large files**: `off_t` is 64-bit where `_FILE_OFFSET_BITS=64` is set
  (check the cmake defines); the copy loop counts bytes, never `size_t`
  arithmetic on file size.

### Interactive prompt (`-i`, and `mv`'s unwritable-destination prompt)

- Write the prompt to `fd_err`, read **one line from stdin** through `fd_in`.
  In an interactive shell `fd_in` is the line editor (`term_read`), which
  would print the PS1 prompt again and offer editing/history; `builtin_read`
  already deals with this (`term_attr(..., &attrs)` around the read,
  `builtin_read.c:104-120`) — reuse that path instead of inventing another.
- Non-interactive with `-i` and stdin at EOF: the answer is "no", the file is
  skipped, status 0 (POSIX: a non-affirmative response is not an error).
- `-f` after `-i` wins (last one determines, POSIX); `-n` and `-i`: `-n` wins
  (GNU/BSD `-n` overrides earlier `-i`; document).

### Shell integration and risks

- **Signals**: a builtin runs inside the shell, and `SIGINT` while copying a
  large file must stop the copy. `cat`/`tee` do not check for it either
  (`tee` even has `-i`). Plan: test between blocks whether the shell's
  interrupt flag is set (find the flag `sh_onsig()` sets; if there is none
  for foreground builtins, that is a general gap — record in `BUGS`), remove
  the partial destination (`mv` fallback temporary; `cp` leaves what it wrote,
  like every `cp`), and return 130.
- **Partial writes / `ENOSPC`**: report the write error, remove nothing that
  existed before, status 1. `mv` never removes the source after a failed copy.
- **`set -C` (noclobber)** does not apply to `cp`/`mv` (it is a redirection
  option), as in bash.
- **Aliases**: `alias cp='cp -i'` works unchanged; `command cp` and
  `builtin cp`… keep `argv[0]`.
- **`hash`/`type`**: they already list builtins from the table; nothing to add.
- **Completion** (Goal 9): the default completion table gets rows for
  `cp`/`mv` (files, then directory for the last operand) with no engine change.
- **Data-loss bug next door**: `ln` currently destroys an existing
  destination with no `-f` (`BUGS: ln-unlinks-existing-destination`, found
  while planning this). `cp`/`mv` must not copy that unlink-first habit: the
  only unlink before writing is `cp -f`'s fallback after `open()` failed.
- **Same-name subtleties**: a source with a trailing slash (`cp -R a/ b`)
  behaves as `cp -R a b` (creates `b`, or `b/a` when `b` is an existing
  directory) — test both; a destination `dir/` with a trailing slash that
  names a non-directory is an error, not a file called `dir`; an empty
  operand `''` gives `cp: cannot stat ''`.
- **Names starting with `-`**: `--` handling only; `cp -- -x y`.
- **`mv` of the current directory or a mount point**: `rename` reports
  `EBUSY`/`EINVAL`; message passes through.
- **Symlink to directory as destination** (`cp f linkdir`, `linkdir` → dir):
  `stat` (following) decides "is a directory", `lstat` is used for the
  destination-exists checks of the final component — same split as coreutils.

### Testing (`tests/builtin-cp.sh`, `tests/builtin-mv.sh`; skipped when not compiled in, as `builtin-digest.sh`)

All assertions through `assert_equal`/`assert_match`, ending with `summary`.
Use `$(mktemp -d)`, `cmp`-free comparisons (shell `read`/`cat` of the file, or
the `digest` builtin when present).

- **`cp`**: file → file (content, and mode = source & ~umask); file → existing
  file (truncated, same inode kept); file → directory; several files →
  directory; `several → non-directory` fails; missing source: status 1 and
  the other sources are still copied; `-R` tree (nested dirs, empty dir, file
  modes, dangling symlink copied as a link with `-P`, followed with `-L`);
  `-R` into itself refused; same file refused (`cp a a`, hard link, symlink
  to it); `-i` with `y`, `n`, EOF; `-n`; `-f` over a read-only destination;
  `-p` keeps mtime (`touch -t` then compare via `ls -l`/`test -nt`) and drops
  set-id when chown is denied; a 5 MiB file and a 0-byte file; names with
  spaces, `-` prefix (`--`), and a trailing slash; unreadable source;
  destination directory not writable; `-v` output format; `-a`.
- **`mv`**: rename in place (inode unchanged); into directory; several into
  directory; over an existing file (contents replaced, `-i` prompt, `-n`);
  dir over non-dir and non-dir over dir refused; `mv a a`; `mv d d/sub`;
  missing source; **cross-device** — use `/dev/shm` (tmpfs) vs `$TMPDIR` on
  disk when they differ (`stat -c %d`); if not, skip that group with a note;
  force the fallback path in a dev-only test hook (`SHISH_CPMV_FORCE_COPY=1`
  compiled under `_DEBUG`), so the EXDEV branch is tested everywhere — copy
  correctness, source removed only on success, failed copy leaves the source
  and no temporary file.
- **Both**: `--` and option combinations (`-fi` vs `-if`, last wins); an
  unknown option → usage, status 1 (or 2? decide with the other builtins:
  they return 1); a directory name that is also an option (`./-r`).
- **Fuzz / differential** (dev-only, not CTest): random trees under a temp
  dir, `shish cp -R` vs the system `cp -R` (or `-RP`), compare with
  `diff -r` plus `find -printf '%m %y\n'`; same for `mv` on `/dev/shm`.
- **WASI**: `tests/builtin-cp.sh` runs unmodified under the Node harness
  (`doc/wasm.md`); record which groups skip.

### Order of work (each step its own change; test + `TODO.md` in the same change)

0. **Fix or record `ln`'s unlink** (`BUGS: ln-unlinks-existing-destination`)
   — one line plus a `tests/fixed.sh` case + `fixes/NN`; it is the model of
   what `cp`/`mv` must not do, and the same "exists?" logic gets written
   twice otherwise.
1. **Export the tree removal**: rename `rm_recursive()` to `builtin_rm_tree()`
   in `builtin_rm.c`, declare it in `builtin.h`, `tests/builtin-rm.sh` unchanged.
2. **`cp` only, through `builtin_cpmv()` with the `cp` row**: options,
   target resolution, regular files, `-i -f -n -p -v`, then `-R` with
   directories, symlinks, FIFOs. `tests/builtin-cp.sh` grows with each.
   Register (`builtin.h`, `builtin_table.c` fallback define + row,
   `cmake/Builtins.cmake` `EXTRA_BUILTINS`, autotools list in `configure.ac`).
3. **`mv`**: `rename` path, prompts, type mismatch, then the `EXDEV`
   fallback on top of `cpmv_copy_tree()` + `builtin_rm_tree()`. **Measure
   both layouts** (dispatch vs two entry points, see the size section) and
   keep the smaller; record the numbers here.
4. **Portability pass**: WASI (Node harness), `cfg-mingw64`
   (`WINDOWS_NATIVE` rename/attributes), musl/diet builds; unguarded calls
   fail the build, not the run.
5. **Signals and large-file behaviour**: interrupt polling, `ENOSPC`, a
   4 GiB sparse file check (skip when the file system lacks holes), then the
   optional `copy_file_range()` fast path if the measured copy speed matters.
6. **Documentation**: `doc/builtins.md` entries, help texts (`help_cp`,
   `help_mv`, kept short — Goal 5.2 counts help bytes), README builtin list.

### How to measure

```sh
# both builtins on: size of the pair
cmake -S . -B /tmp/cpmv -DCMAKE_BUILD_TYPE=MinSizeRel -DENABLE_CP=ON -DENABLE_MV=ON
cmake --build /tmp/cpmv -j8
size /tmp/cpmv/CMakeFiles/libshell.dir/src/builtin/builtin_cpmv.c.o
nm --size-sort -S /tmp/cpmv/CMakeFiles/libshell.dir/src/builtin/builtin_cpmv.c.o | tail -20
strip /tmp/cpmv/shish && stat -c%s /tmp/cpmv/shish     # vs the same build with both OFF
# mv only / cp only: the row that is off must not pull its code (gc-sections check)
# the copy engine's speed against coreutils
dd if=/dev/urandom of=/tmp/big bs=1M count=512; time /tmp/cpmv/shish -c 'cp /tmp/big /tmp/big2'; time /bin/cp /tmp/big /tmp/big3
```

---

## Goal 11 (secondary) — `sed` and `awk` builtins, in depth (as small as possible, maximum reuse)

**`sed`: done (commit `c3ae2fa5`). `awk`: done (2026-09-25) — see the
"Status" note near the end of this section for what shipped and what
didn't; the rest of this section is still an accurate read of the
design both were actually built from.** This section refines Goal 6
(which plans `lib/dfa`, `expr`, `grep` and a first `sed` estimate) and
adds `awk`. Specs read 2026-09-20: POSIX.1-2024
[`sed`](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/sed.html) and
[`awk`](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/awk.html).

**`sed` is done** (`text/sed/` + `src/builtin/extra/builtin_sed.c`,
`c3ae2fa5`), including the shared foundation pieces it needed:
`dfa_replace`/`dfa_repl` (`text/dfa/dfa_replace.c`) and `lib/arena`
(`696bdcce`, see Goal 3). **`awk` is done too** (`text/awk/` +
`src/builtin/extra/builtin_awk.c`, `23998969`, 2026-09-25). The design below
(originally written for both) is the record of what both were built from;
treat the shared-foundation table just below as "already built and in use."

### Answers to the five questions

| Question | Answer |
|---|---|
| **Is `lib/dfa` suited to `awk`?** | **Yes, for the regex half only, and it needs *less* of it than `sed`.** awk uses ERE, never back-references, never `\1`; `sub`/`gsub` replace with `&` (whole match) only. So awk needs `dfa_compile` + `dfa_test` (`~`, patterns) + `dfa_search` (`match`, `sub`, `gsub`, regex `FS`/`split`) and **not** `dfa_submatch` or the backtracker, which the linker then drops (`--gc-sections`). The lazy DFA is also the right engine for awk's usage pattern: the same few regexes run against every record, so the state cache built on record 1 serves the rest of the file. |
| **Do `sed` and `awk` need recursive-descent parsing?** | **`sed`: no.** Its language is flat: `[addr[,addr]]cmd[args]` per line, nesting only through `{ }` (an explicit stack of open braces; each `{` stores the index of its `}`) and forward branch targets (labels resolved by a fix-up pass). One loop over the script, no recursion. **`awk`: yes, but little.** Statements nest (`if`/`while`/blocks/functions) so the statement parser recurses; expressions have 16 precedence levels but are parsed by **one** precedence-climbing function driven by a table, not 16 functions. No parser generator exists in-tree, and the POSIX grammar is a yacc grammar with lexical feedback (regex-vs-`/`, `getline`, newline rules); a hand-written parser is smaller than yacc tables plus a lexer that must cooperate with them. |
| **Can they be implemented on top of `dfa`?** | `dfa` is the **regex layer** under both, nothing more: it does not parse either language and does not carry state between lines. Everything else (script/program parsing, execution cycle, values, fields, I/O) is new code. What both want from `dfa` beyond Goal 6 is one shared helper, **`dfa_replace()`** (the "find next match, copy the gap, append the replacement, advance past empty matches" loop that `s///` and `sub`/`gsub` are both made of), so that loop is written once. |
| **`awk`: build a real AST, or compile straight to bytecode?** | **AST, tree-walking interpreter** -- same shape as the shell's own `union node` parser: one parse produces a tree, execution (`eval/`) and pretty-printing (`sh_fmt.c`/`shformat`) are two separate walks over the *same* tree, and `shparse2ast` is a third (a dumper). An `awk_node` tree gets an `awkformat`/`awkparse2ast` the same way, free of any extra parsing work. `text/dfa` went straight to bytecode instead, but for a reason that doesn't generalize: a compiled regex runs over and over with no legitimate "print the pattern back out" use case (the source pattern already *is* the readable form) -- awk programs are the opposite, short scripts worth reformatting. Bytecode-first forecloses that cheaply; AST-first doesn't foreclose bytecode -- an AST-to-bytecode compiler is a natural later stage *on top of* the tree if tree-walking ever proves too slow (unlikely at this project's stated proof-of-concept bar; the shell interpreter is itself a tree-walker despite running in loops). Recovering a formattable structure from flat bytecode after never building a tree is a decompiler, strictly harder -- not worth setting up as the starting position. |
| **Can awk parsing share code with `src/parse/parse_arith_*.c`?** | **Not directly; at most a table-driven core later** — see "Sharing with the shell's arithmetic parser" below. Short version: the arithmetic parser is a character-level parser over the shell's global `source`, `int64` only, building shell `union node`s and evaluated against the shell variable table; awk needs a token stream, `double`+string dual values, a different operator set, and context-dependent lexing. Nothing but the *idea* (precedence climbing) transfers. |
| **How big?** | Estimates below, in this repo's line style (≈ 40 % blank/comment/brace lines; calibration ≈ 7-10 B of code per line, Goal 6): **`sed` ≈ 850 lines / 7-9 KB** (was ≈ 1200 in Goal 6), **`awk` ≈ 2100 lines / 16-21 KB** (+ libm on dynamic-link-free builds), on top of ≈ 400 lines of shared/imported foundation. For scale: `../busybox/editors/awk.c` is 4028 lines and `sed.c` 1684 (both with GNU extensions and libc `regex`). |

### Things found while researching (2026-09-20) that shape the plan

- **`awk` needs floating point; the shell has none.** Nothing in `src/` uses
  `double`, `strtod` or `snprintf`. Awk numbers are `double`, and CONVFMT/OFMT
  (`"%.6g"`) rounds, so exact conversion matters for output.
- **`c-utils`' `scan_double`/`fmt_double` are not usable**, measured:
  - `scan_double("0.3")` gives `0.30000000000000004`, libc `strtod` gives
    `0.29999999999999999` (it multiplies by a running `0.1` factor), so a
    program's literal `0.3` would not equal the field `0.3`, or `0.1+0.2`.
  - `fmt_double(0.3, prec 6)` prints `0.299999` (truncates, does not round),
    `100` prints `100.000`, `1e21` prints `1.00000e21`, `1.234e-06` prints
    `0.123399`; it also reads its union before assigning it.
  - **Decision: libc `strtod` and `snprintf("%.*g")`, in two small wrappers**
    (`awk_str2num`, `awk_num2str`). Integer-valued doubles (POSIX: converted
    "as if `%d`") are formatted with `fmt_longlong` and never touch
    `snprintf`, so the common case (`NR`, `NF`, counters) costs no libc
    float code. `setlocale` is never called, so the radix is always `.`
    (POSIX says awk uses `LC_NUMERIC`; shish is always in the C locale).
- **`builtin_printf.c` already has the format machinery awk needs** (flags,
  width, precision, `*`, the numeric padding), but it writes straight to
  `fd_out` and is integer-only. Refactoring it to write into a `stralloc*`
  sink and exposing `printf_parse_spec`/`printf_emit_numeric` lets awk's
  `printf`/`sprintf` reuse ≈ 150 lines of it; awk adds `%e %E %f %F %g %G`
  by building the spec text and calling `snprintf` for just those.
- **`lib/arena` is implemented** (`696bdcce`, see Goal 3; `arena_init/alloc/
  reset/free` plus `tell/rewind/grow/trim`). It serves this goal
  and Goals 3/4: the sed script and the awk program live in one arena freed
  at exit; awk's *temporaries* (string results of expressions) live in a
  second arena reset after every statement, so the interpreter needs almost
  no `free()` logic (≈ 100 lines saved, and a class of leaks).
- **`c-utils` has what the containers need**: `hashmap` (10 files, 164 lines
  in total) for awk arrays; `buffer_getline_sa` (7 lines) and
  `buffer_get_token_sa` (29) for records — `lib/stralloc.h` already
  *declares* both but the `.c` files are missing (link error today).
- **`busybox/editors/awk.c` as a yardstick** (4028 lines; largest functions,
  measured): `evaluate` 711, `debug_parse_print_tc` 475 (debug only),
  `awk_printf` 273, `next_token` 215, `parse_expr` 199, `exec_builtin` 191,
  `awk_sub` 113, `awk_getline` 97, `awk_split` 86. The plan below reaches the
  same feature set with the regex engine, the `hashmap`, the printf formatter
  and the arena imported instead of written inline.

### Shared foundation (build once, used by both)

| Piece | Source | State | Lines | Used by |
|---|---|---|---|---|
| `text/dfa`: `dfa_compile/test/prefix/search/submatch` | Goal 6 | **done** | (Goal 6) | sed (all), awk (compile/test/search) |
| **`dfa_replace()`** — global/nth substitution loop with the empty-match rule (`s/x*/-/g` on `abc` gives `-a-b-c-`) | `text/dfa/dfa_replace.c` | **done**, in use by sed `s///` | ≈ 70 | sed `s`, awk `sub`/`gsub` |
| replacement compiler: `&`, `\&`, `\\`; `DFA_REPL_BACKREF` also gives `\1`-`\9` and `\n` | `text/dfa/dfa_replace.c` (`dfa_repl_compile`, same file as `dfa_replace()`, not a separate `dfa_repl.c`) | **done**, in use by sed `s///` | ≈ 45 | sed, awk |
| `arena_*` | `lib/arena.h` + `lib/arena/` | **done** (`696bdcce`; see Goal 3) — one caller so far, `text/dfa/dfa_run.c`'s per-search save arrays; not yet adopted by sed's own parse-scratch (label/fixup tables in `sed_parse.c`) or awk | ≈ 60 | sed script, awk program + temporaries |
| `hashmap_*` (awk arrays, ENVIRON, output-file table) | `../c-utils/lib/hashmap/` | copy | 164 (+ ≈ 10 for a `hashmap_next` iterator) | awk |
| `buffer_getline_sa`, `buffer_get_token_sa` | `../c-utils/lib/stralloc/` | copy | 36 | sed lines, awk records (`RS` single char) |
| `byte_lower`/`byte_upper`, `byte_findb` | `../c-utils/lib/byte/` | copy | ≈ 30 | awk `tolower`/`toupper`/`index`, sed `I` flag helpers |
| printf formatter to a `stralloc` sink | refactor `src/builtin/builtin_printf.c` | refactor | −0 net for the shell; awk saves ≈ 150 | awk `printf`/`sprintf`, shell `printf` |
| `fmt_escapecharcx`/`fmt_escapecharnonprintable` | `../c-utils/lib/fmt/` | copy | ≈ 60 | sed `l` |
| UTF-8 `mb_*` (Goal 7) | planned | — | — | character semantics of `length`, `substr`, `index`, `match`, `printf %c` (Issue 8 awk is character-based; bytes are conformant in the POSIX locale, so this is a later milestone) |

Deliberately **not** used: `cbmap` (847 lines; its one advantage, ordered
iteration, is not worth 5× the code — POSIX leaves `for (k in a)` order
unspecified), `array` (357 lines; a `stralloc` of fixed-size records does the
job), `tokenizer.h` (a C-language tokenizer: no regex-vs-`/` handling, no
newline tokens), `scan_double`/`fmt_double` (see above).

### `sed` — design

**Structures**

```c
struct sed_addr { unsigned char kind; /* NONE LINE LAST REGEX */ unsigned long n; struct dfa* re; };
struct sed_cmd  {                    /* fixed size; the array lives in a stralloc */
  unsigned char op, neg, inrange;    /* command letter, '!', range is open */
  struct sed_addr a1, a2;
  unsigned long endline;             /* a2 as a line number once resolved */
  union { int target;                /* '{' -> index of '}', b/t -> index of the label */
          char* text;                /* a i c r w  (arena) */
          struct sed_subst* s;       /* s: regex, replacement pieces, flags, nth, wfile */
          unsigned char* y;          /* y: 256-byte table */ } u;
};
```

- **Script assembly**: `-e` and `-f` pieces are concatenated in order, a newline
  inserted after a `-e` piece (POSIX); `#n` as the first two characters of the
  first piece is `-n`. One `stralloc`; the parser reads it with an index.
- **Parser** (≈ 330 lines): a single `for(;;)` that skips blanks/`;`/newlines,
  parses `addr[,addr]`, an optional `!`, the command letter, then its
  argument; `{` pushes the command index, `}` patches it. Labels are recorded
  in a small table; after the last command every `b`/`t` operand is resolved
  by a fix-up loop (unknown label is an error; labels are unique to at least 8
  bytes per POSIX, we compare the whole name). Regex delimiters: `\cREc`
  and `s` use one `sed_delim()` that removes the backslash from `\c` when `c`
  is the delimiter (POSIX Issue 8: the delimiter does not end the RE inside a
  bracket expression), and turns `\n` into a newline — then hands the text to
  `dfa_compile(..., DFA_SED | (E ? DFA_ERE : 0) | (I ? DFA_ICASE : 0))`. An
  empty pattern means "the last regex used at run time": the command keeps
  `re == NULL` and the executor uses `sed_last_re`.
- **Execution** (≈ 260 lines): one loop over the command array with a program
  counter (`{` not selected jumps over its body; `b`/`t` set the counter),
  wrapped in the cycle: read line → run → autoprint (unless `-n`) → flush the
  append queue. State: pattern space and hold space (`stralloc`), line number,
  `t` flag, `quit`, the append queue (a list of `a` text and `r` file names, in
  order), the `D` restart flag ("start the next cycle without reading input"),
  and the last-line lookahead.
- **Address matching** (≈ 50 lines): a range keeps `inrange` in its command.
  Rules from the spec: a second address that is a number ≤ the first line
  selects only that line; a regex second address is tried from the line
  *after* the first; `addr!cmd`; `$` needs the lookahead.
- **Input** (≈ 90 lines): the operand list, `-` or none = stdin, files opened
  one at a time; a **one-line lookahead** across files answers "is this the
  last line" (an unreadable or empty *later* file must not make the current
  line non-last: skip ahead to the next readable line, diagnose the bad file
  once). Line numbers are cumulative across files. Missing final newline:
  see Goal 6 (output always ends in a newline, POSIX).
- **Commands** (executor `switch`, ≈ 150 lines): `= a b c d D g G h H i l n N p P
  q r s t w x y : { } #`. `n`/`N` at end of input: `N` quits **without**
  printing (spec text, and Issue 8's clarified timing: the append queue is
  flushed just before the next input fetch by `c D d N n`, before `q`, and at
  the end of the script); `n` quits after autoprint. `c` starts the next cycle
  for **every** line of its range (Issue 8 defect 1767), printing the text
  only at a single address or at the end of a range. `l` folds at 70 columns
  and ends with `$`. `w` files are created before input is read, shared by
  name, `/dev/stdout` is special.
- **`s`**: the replacement is compiled once into pieces (literal / group `n` /
  whole match) by `dfa_repl`; flags `g p w i n` and the number; `g` with a
  number is unspecified in POSIX (we do "nth and after", like GNU). The
  executor calls `dfa_replace()`; groups are asked from the backtracker only
  when a piece needs `\1`-`\9` (a pattern with no captures and a replacement
  of only `&`/text stays on the pure DFA).
- **Not implemented** (documented): `-i`, `-s`, `-z`, `e F z W R M`, `0,/re/`,
  `addr,+N`, `first~step`, `q`/`Q` exit-code arguments, the `y` escapes other
  than `\n`, `\\` and the delimiter (undefined in POSIX). Unknown command:
  `sed: -e expression #1, char N: unknown command: X`, status 1.

**Sizes (`sed`)** — likely / range, lines: options + script assembly 90;
parser 330 (addresses 70, command switch 110, `s` 90, `y` 30, text/label/wfile
30); regex glue 40; executor 260; input/output/append queue/wfiles 130; `l` 35;
**≈ 885 (700-1000)** = ≈ 7-9 KB. The drop from Goal 6's ≈ 1200 comes from
`dfa_replace` + `dfa_repl` (≈ 80 lines it would have inlined), the
flat parser (no recursion, no separate token layer), `buffer_getline_sa`
instead of a private reader, and `arena` instead of per-command `free()` code.

### `awk` — design

**Pipeline**: `source text → lexer (on demand, 1 token of lookahead) → parser →
tree of struct anode in the program arena → interpreter (tree walk)`.

**Lexer** (≈ 220 lines). Tokens: `NUMBER STRING ERE NAME FUNC_NAME BUILTIN
KEYWORD NEWLINE` and operators (two-character ones from the grammar: `+= -= *= /=
%= ^= || && !~ == <= >= != ++ -- >>`). The context rules from the spec, each
one a few lines:

- **`/` is division unless a value cannot end the previous token**:
  keep `prev` (class of the last token); if it is `NAME NUMBER STRING ) ] $ ++ --
  BUILTIN` (something that ends an operand) a `/` is `/` or `/=`, otherwise it
  starts an ERE. (POSIX wording: "where the token `/` or `DIV_ASSIGN` could
  appear as the next token … the longer of those two".)
- **`FUNC_NAME`**: a name immediately followed by `(` (no blank).
- **Newlines**: a token of its own; `\`+newline is skipped; the parser, not the
  lexer, decides where a newline may be ignored (after `{ && || , do else`, after
  `)` of `if/for/while`, after `;`).
- **Strings**: escapes `\\ \" \/ \a \b \f \n \r \t \v \ddd`; a raw newline in a
  string is an error. **EREs**: the text between the slashes with `\/`
  turned into `/`; everything else is passed to `dfa_compile` untouched (its
  ERE syntax already handles `\.` and brackets; awk's `\ddd`, `\n` and friends
  are turned into the byte first by one 20-line pass shared with string
  literals). A dynamic regex (`$0 ~ s`) gets the same pass because the string
  value `"\\."` has already been through string escape processing once.
- **Numbers**: decimal with optional fraction and exponent (`strtod` on the
  scanned span); hexadecimal constants are optional in POSIX and not accepted.
- **Comments** to end of line; keywords: `BEGIN END function func if else while
  for do break continue next nextfile exit return delete in getline print printf`;
  built-in function names are their own token class.

**Parser** (≈ 540 lines).

- **Program**: items until EOF; `BEGIN`/`END` bodies (several allowed, kept in
  order), `pattern`, `pattern action`, `expr , expr action` (range),
  `function name(params) action`. A missing action is `{ print }`.
- **Statements** (≈ 200 lines): `{ }`, `if/else`, `while`, `do … while`,
  `for(;;)`, `for (k in a)`, `break continue next nextfile exit [expr]
  return [expr]`, `delete a[i]` / `delete a`, `print`/`printf` with an optional
  `> >> |` target, and expression statements. Terminators are `;` or newline
  or the enclosing `}`.
- **Expressions** (≈ 260 lines): **one function, `awk_expr(min_prec)`**,
  precedence climbing over this table (highest first; from the spec):

  | Prec | Operators | Assoc |
  |---|---|---|
  | 14 | grouping `( )`, `$expr`, `++x --x x++ x--` (prefix/postfix, in the primary) | — |
  | 13 | `^` | right |
  | 12 | unary `! + -` | (prefix) |
  | 11 | `* / %` | left |
  | 10 | `+ -` | left |
  | 9 | concatenation (adjacency) | left |
  | 8 | `< <= != == > >=` | none |
  | 7 | `~ !~` | none |
  | 6 | `in` | left |
  | 5 | `&&` | left |
  | 4 | `\|\|` | left |
  | 3 | `?:` | right |
  | 2 | `= += -= *= /= %= ^=` | right (lvalue on the left) |

  The awk-specific cases live in the few lines around the loop, not in the
  table: **concatenation** has no token, so "the next token starts a primary and
  is not a binary operator" continues the loop as a concatenation;
  **unary minus vs `^`** (`-2^2` is `-4`, `2^-3` works): the operand of unary
  `- + !` is parsed at prec 12, the right side of `^` allows a unary operator;
  **`in`** takes a parenthesised subscript list on the left (`(i,j) in a`);
  **`>` inside `print`/`printf`** is a redirection unless parenthesised
  (a `no_gt` flag threaded down, cleared inside `( )`); **`getline`**:
  `getline`, `getline var`, `getline < file`, `getline var < file` (the file
  operand is a *primary-level* concatenation-free expression, as in the
  grammar) and `cmd | getline [var]` (a postfix step after a full `|` left
  side); **regex literal as an expression** is `$0 ~ /re/` (an `ERE` node
  outside `~ !~` and outside built-in regex arguments).
- **Names are resolved at parse time**: globals get an index into one array
  (names interned through a `hashmap`), function parameters get a frame slot,
  so the interpreter never looks a name up by string. The special variables
  (`NF NR FNR FS OFS ORS RS SUBSEP CONVFMT OFMT RSTART RLENGTH FILENAME ENVIRON
  ARGC ARGV`) occupy fixed indices below `NSPECIAL`; after every assignment
  `if(idx < NSPECIAL) special_assigned(idx)` (≈ 15 lines) handles the hooks
  (`NF`, `FS`, `$0`).
- **Node**: `struct anode { short op; short flags; union { double num; char* str;
  struct dfa* re; int idx; } u; struct anode *a, *b, *c, *next; }` (≈ 48 bytes),
  all in the program arena.

**Values and conversions** (≈ 160 lines)

```c
struct cell {                 /* a value; also a variable slot */
  unsigned char type;         /* UNINIT NUM STR STRNUM(a string that looks numeric, decided lazily) ARRAY */
  double num;
  char*  str;                 /* NUL-terminated; arena temp or heap, by owner */
  struct hashmap* arr;        /* ARRAY */
};
```

- **Strings are C strings** (POSIX: a NUL in a pattern, record or string gives
  undefined results), so no length field and `str_len`/`byte_*` work directly.
- **Number → string**: an integer-valued double becomes `%d` (`fmt_longlong`), any
  other uses CONVFMT (`snprintf`), OFMT for `print`. **String → number**:
  `strtod` on the leading numeric prefix. **Numeric-string** (from input, `getline`,
  `FS`-split fields, `ARGV`, `ENVIRON`, `-v`/command-line assignments, `split()`):
  decided when *compared* by one function `looks_numeric(s)` (blank-trimmed,
  optional sign, digits, one optional radix, optional exponent, nothing left).
- **Comparison**: numeric if both are numbers or numeric strings or one is a
  number and the other uninitialised; otherwise string comparison
  (`==`/`!=` by identity, `<` etc. by `strcmp` — C-locale collation, POSIX
  says `strcoll`). Truth: number != 0, string non-empty.
- **Temporaries**: expression results that are strings are copied into the
  *statement arena*, reset after every statement; assignment copies into the
  variable's own heap string. `eval()` therefore never frees.

**Interpreter** (≈ 560 lines): `struct cell eval(struct anode*)` is one switch
(≈ 220 lines: arithmetic, comparison, logic, assignment/compound
assignment/incr-decr through `lvalue()`, `$`, concatenation, `?:`, `in`, `~`,
calls, `getline`, built-ins); `int exec(struct anode*)` returns a control code
(`NORMAL BREAK CONTINUE NEXT NEXTFILE EXIT RETURN`) so loops and functions need no
`longjmp`; **`exit` inside an expression-called function and fatal runtime errors
use one `setjmp` in the builtin** (division by zero, bad regex, write error:
`awk: division by zero`, status 2). User functions: a frame is an array of
`cell` for `params`; scalars are copied, an array argument shares the
`hashmap*`; missing arguments are uninitialised locals; an uninitialised local
passed on as an array becomes an array in the caller (the POSIX rule, ≈ 15 lines
in the call code); recursion depth is limited (default 1000) to fail cleanly
instead of overflowing the C stack.

**Records and fields** (≈ 220 lines): one `struct rec { stralloc line; char** f;
size_t nf; flags }`, split **lazily** (on the first `$n`/`NF`) with the `FS`
in force *when the record was read* (spec): default `" "` (blanks and newlines
trimmed and collapsing), single character (each occurrence, except that `\t` etc.
work), otherwise an ERE through `dfa_search`. `$0` is rebuilt lazily with `OFS`
after a field or `NF` assignment; assigning `$0` resplits; `$(NF+2)=x` grows
`NF`. `RS`: first character, or `""` = paragraph mode (blank-line separated,
newline always a field separator) — implemented in `awk_getrec` on top of
`buffer_get_token_sa`; a regex `RS` is an extension, off.

**Built-in functions** (each one small; `length`, `substr`, `index`, `match`,
`printf %c` become character-based with Goal 7):

| Function | Strategy | Lines |
|---|---|---|
| `length [(s)]`, `length(arr)` | `str_len` (or element count) | 8 |
| `substr(s,m[,n])` | POSIX rounding rules (`m`, `n` rounded, clipped to 1..len) | 25 |
| `index(s,t)` | `byte_findb` | 6 |
| `match(s,re)` | `dfa_search`; sets `RSTART`, `RLENGTH` (0 / -1) | 15 |
| `split(s,a[,fs])` | same splitter as fields; clears `a`; elements are numeric strings | 30 |
| `sub`/`gsub(re,repl[,target])` | `dfa_replace` + `dfa_repl` (awk dialect); target defaults to `$0`; writes back through `lvalue()` | 45 |
| `sprintf` | shared printf formatter (see below) | 25 |
| `tolower`/`toupper` | `byte_lower`/`byte_upper` on a temp copy | 10 |
| `sin cos atan2 exp log sqrt int` | libm, `int` = `trunc` | 25 |
| `rand`/`srand` | 64-bit LCG; `srand()` seeds from `time()`; returns the previous seed | 20 |
| `close fflush system` | see I/O | 40 |

**`printf`/`sprintf`** (≈ 100 lines on top of the refactored formatter): walk the
format; `%d %i %o %x %X %u %c %s %%` go through the shared code with the argument
converted (`%c`: a number is a byte value, a string its first character); `%e %E
%f %F %g %G` build a spec string and call `snprintf`; `*` width/precision take
the next argument; too few arguments is undefined in POSIX — we treat the
missing ones as uninitialised.

**Input/output and processes** (≈ 140 lines + the shell hook)

- `getline` forms as in the spec, returning 1/0/-1; plain `getline` sets
  `$0 NF NR FNR`, `getline var` sets `var NR FNR`, `getline < file` sets `$0
  NF` (or `var`) only. Files stay open in a `hashmap` keyed by name until `close`.
- **Output redirections** `> >> |`: a `hashmap` of open outputs by target string;
  closed by `close()` and at exit; `fflush` flushes one or all.
- **`system(cmd)` and `cmd | getline`** run through **the shell's own
  evaluator** (the machinery behind `$(...)`/`eval`), not libc `system()`/
  `popen()`: builtins are available to the command, it works where there is no
  `/bin/sh`, and no second process model is introduced. `print | cmd` needs a
  writable pipe to a *process*, i.e. `fork`: available natively, **not on WASI**
  (`doc/wasm.md`), where it is a runtime error. Standard output is flushed
  before running a command so output order matches.
- `ENVIRON` from `var_export()`; `ARGV`/`ARGC` from the operands, assignments
  (`name=value`) processed when the argument is reached (escape sequences in
  the value are processed, as for `-v`).

**Main and options** (≈ 120 lines): `-F sep` (`-Ft` means tab is *not*
special in POSIX: keep literal `t`), `-f progfile` (repeatable, `-` = stdin,
concatenated), `-v assign`, `--`; a BEGIN-only program never opens input;
exit status: `exit expr`, else 0, else 2 for a runtime error; syntax errors
print `awk: syntax error at source line N` and exit 2.

**Decisions for POSIX's "unspecified" points** (each a test): `for (k in a)`
order = hash order (documented, not sorted); `SUBSEP` = `\034`; `srand()` with no
seed uses `time(0)`, `rand()` unseeded starts from seed 0 (deterministic);
`uninitialized` vs `""` distinguished only where the spec requires (comparisons);
`FS` null: each character is a field (GNU/onetrue behaviour); `RS` multi-character:
first character only; `printf` with too few arguments: empty/0; `NaN`/`inf` printing
whatever libc gives; `substr` with NaN: empty; division by zero: fatal error;
assignment to `NR/NF/FNR` allowed; `getline` from a directory: -1; regex
special-casing of `NUL`: unsupported (C strings).

### Sharing with the shell's arithmetic parser (`src/parse/parse_arith_*.c`)

Measured today: `parse_arith_*` = 603 lines, `expand_arith_*` = 321 lines
(≈ 924 lines, ≈ 7-9 KB in the **default** build, because `$(( ))` and `expr` are
default builtins).

| Aspect | Arithmetic parser | awk | Shareable? |
|---|---|---|---|
| Input | characters via `source_peek`/`source_next` (a global buffer with pushback) | tokens from an awk lexer over a string | no |
| Lexing | digits/hex/octal by hand in `parse_arith_value` | decimal floats, strings, EREs, names, keywords, newlines | no |
| Values | `int64` only | `double` + string + strnum + arrays | no |
| Operators | `** * / % + - << >> < <= > >= == != & ^ \| && \|\| ?: = op= ++ --` | `^ * / % + - concat < <= != == > >= ~ !~ in && \|\| ?: = op= ++ -- $` | overlap ≈ 60 % but different precedence (awk has no bit operators, and `^` is exponent) |
| Tree | shell `union node` (`narithbinary` …), freed by `tree_free` | `struct anode` in an arena | no |
| Evaluation | `expand_arith_*` against the shell variable table (`var_setv`) | interpreter against awk cells | no |
| Method | recursive precedence walk (`parse_arith_binary(p, prec)` calls itself with `prec-1`) | precedence climbing | **the idea only** |

**Recommendation:** do not couple them in the first pass. Write awk's
`awk_expr(min_prec)` as a small, self-contained function of *(operator table,
`next_token()`, `mk_node()`)* so that it can be lifted. **Optional Phase 8**
(only if measured to help): move it to `src/parse/parse_prec.c`, give the
shell arithmetic a small token lexer instead of `source_peek` character
juggling, and rebuild `parse_arith_*` on the shared core; the expected gain is
**≈ 200 fewer lines / ≈ 1.5 KB in the default build**, at the price of
touching the default `$(( ))`/`expr` path, so the gate is the existing arithmetic
tests (`tests/param.sh`, the conformance `.tst` files that use `$(( ))`) plus a
differential test of both parsers over a large random expression set before the
old one is deleted. If that is not attractive the awk core stays private and
nothing is lost.

### Reuse summary (what is written, what is copied, what is refactored)

| | `sed` | `awk` |
|---|---|---|
| **New** | parser, cycle/executor, address/range logic, append queue, `l`, wfile table (≈ 885 lines) | lexer, parser, interpreter, values, fields/records, built-ins, I/O glue, main (≈ 2100 lines) |
| **From `lib/dfa`** | compile, test, search, submatch, `dfa_replace`, `dfa_repl` | compile, test, search, `dfa_replace`, `dfa_repl` |
| **Copied from `../c-utils/lib`** | `buffer_getline_sa`, `fmt_escapecharcx`/`nonprintable` | `hashmap`, `buffer_getline_sa`, `buffer_get_token_sa`, `byte_lower/upper`, `byte_findb` |
| **Already in `lib/`** | `stralloc`, `buffer`, `open_*`, `scan_*`, `fmt_*`, `path_fnmatch` (via dfa) | same, plus `fmt_longlong`, `str_*`, `byte_*` |
| **Implemented once for both** | `arena` | `arena` |
| **Refactored** | — | `builtin_printf.c` (stralloc sink) |
| **libc** | none beyond what the shell uses | `strtod`, `snprintf` (non-integer numbers, `%e %f %g`), libm |

### Size estimates (lines in this repo's style; bytes at ≈ 7-10 B/line)

| Component | min | likely | max |
|---|---|---|---|
| **`sed`** | 700 | **885** | 1000 |
| — bytes | | ≈ 7-9 KB | |
| **`awk`** lexer | 170 | 220 | 280 |
| parser (program 80, statements 200, expressions 260) | 450 | 540 | 650 |
| nodes, name resolution, arena glue | 60 | 80 | 110 |
| values and conversions | 120 | 160 | 210 |
| interpreter (eval 220, exec 160, calls 90, lvalues 90) | 480 | 560 | 680 |
| records and fields | 170 | 220 | 280 |
| built-in functions (string 200, math 45, misc 40) | 240 | 285 | 350 |
| `printf`/`sprintf` + number formatting wrappers | 80 | 100 | 140 |
| I/O (`getline`, redirections, `close`, `fflush`, `system`) | 110 | 140 | 200 |
| main, options, ARGV/ENVIRON | 90 | 120 | 160 |
| **`awk` total** | **1970** | **2425** | **3060** |
| — after the shared parts are imported instead of written | | **≈ 2100** (1700-2600) | |
| — bytes | | ≈ 16-21 KB, plus libm where it is not already linked | |
| **Shared foundation** (`dfa_replace` + `dfa_repl` 115, arena 60, hashmap 174, readers 36, byte 30, printf refactor ≈ +40, `l` helpers 60) | | **≈ 515** | |

The `awk` rows above the "after" line count what each component costs if
written standalone; the "after" figure subtracts what the shared foundation
provides (`dfa_replace`/`dfa_repl` ≈ 115 for `sub`/`gsub`, `hashmap` 174 for
arrays and the output table, the printf formatter ≈ 150, the record reader
≈ 40, arena ≈ 60). Cut list if it must shrink further (each ≈ −40 to −120 lines):
no `nextfile`, no `getline < file`/`cmd | getline`, no `srand` seed return, no
`printf` `*`, `length(arr)` only.

### Order of work (each step its own change with tests; `TODO.md`/`BUGS` updated in the same change)

0. **Prerequisites (Goal 6 steps 1-4)**: `lib/dfa` with `dfa_search`; then in `lib/`: implement `arena`, copy `hashmap`,
   `buffer_getline_sa`, `buffer_get_token_sa`, `byte_lower/upper/findb`; write
   `dfa_replace`/`dfa_repl` with their table test (`tests/dfa.sh` gets rows for
   the empty-match rule, `&`, `\&`, `\1`, nth occurrence).
1. **`sed` core**: script assembly, parser for addresses/`{}`/`p d q =`, cycle and
   input with the last-line lookahead, `-n`, `-e`, `-f`, `-E`. Tests from the spec
   examples.
2. **`sed` `s`**: replacement, flags `g p w N i`, empty-match rule, empty regex =
   last regex. Then `h H g G x n N D P`, then `a i c r w y l`, `: b t`, `#n`, the
   Issue-8 timing rules for the append queue, `N` at EOF. `tests/builtin-sed.sh`.
3. **Printf refactor** (independent, small): move `printf_spec`/parse/emit into a
   shared internal file writing to a `stralloc`; `tests/` for the shell `printf`
   must be unchanged.
4. **`awk` front end**: lexer + parser + `awk --dump-ast` (dev switch, `_DEBUG`
   only) with a corpus of programs that must parse and programs that must not (the
   POSIX grammar's edge cases: `/` vs regex, `getline` forms, `print > expr`,
   `a b c` concatenation, `-x^2`, newlines, `;`).
5. **`awk` core interpreter**: `BEGIN`/`END`, patterns and ranges, fields, `print`,
   variables, arrays, control flow, user functions; `tests/builtin-awk.sh`
   grows per feature.
6. **`awk` built-ins and `printf`**: string functions, math, `sprintf`/`printf`
   including `%e %f %g`, `substr` rounding, `split`, `sub`/`gsub`, `match`.
7. **`awk` I/O**: `getline` forms, redirections, `close`/`fflush`, operands with
   assignments, `RS=""`, `ENVIRON`, `system`, `cmd | getline`; `print | cmd` where
   `fork` exists.
8. **Character semantics** with Goal 7 (`length`, `substr`, `index`, `match`,
   `printf %c` by characters); **optional** regex `RS`; **optional** arithmetic
   retrofit (see above).
9. **Register** both builtins (`EXTRA_BUILTINS`, off by default; `builtin.h`,
   `builtin_table.c`, `cmake/Builtins.cmake`, autotools list), help text, docs; measure
   the sizes and update the tables here.

### Testing

- **`sed`**: a table of `script|input|expected` rows (`tests/builtin-sed.sh`): the
  POSIX examples, `tac` (`1!G;h;$!d`), `wc -l` (`-n '$='`), joining lines
  (`:a;N;$!ba;s/\n/ /g`), every command and flag, ranges (`2,4`, `/a/,/b/`, `2,1`,
  `0`-style edge), empty regex reuse, `-n`/`#n`, `N` and `n` at EOF, `c` in a range,
  `D` restart, `y` with escapes, `l` folding, `w /dev/stdout`, multi-file `$` and
  cumulative line numbers, a missing final newline, an unreadable second file.
- **`awk`**: table rows `program|input|expected` (`tests/builtin-awk.sh`): the
  classic one-liners (field sums, word count, associative counting, joining,
  reversing lines), the **type matrix** (`"10" < "9"`, `$1 == 0` on `0`, `0.0`, `" 0"`,
  `"0x"`, uninitialised vs `""`, numeric-string propagation through assignment and
  `split`), `substr` edge cases (0, negative, NaN, fractional), `printf` (`%c` with
  a number and a string, `%5.2f`, `%-5d|`, `%*d`, `%e`, `%g`, `%%`, `%i`), `getline`
  loops from a file and from a command, `close` and re-read, `-v` and operand
  assignments with escapes, `RS=""` paragraph mode with `FS`, `NF` and `$0`
  rebuilding (`$3=""`, `NF=2`, `$(NF+2)=x`), regex `FS`, `FS=","`, tab, `" "`,
  user functions (recursion, array parameters, uninitialised local as array),
  `delete`, `in` with `SUBSEP`, `exit` in `BEGIN`/`END`/function, `next`/`nextfile`,
  `srand`/`rand` range, division by zero.
- **Differential** (dev-only, not CTest): random *valid* programs from a small
  generator (the Goal 8 fuzzer's word synthesis can seed it) run through `shish awk`
  and `gawk --posix`/`mawk`; compare stdout and status, skipping the documented
  unspecified points. External corpora, if available on the machine and not
  vendored: `gawk/test/*.awk` with their `.ok` files, `onetrue-awk/testdir/T.*`.
- **Parser robustness**: random bytes as a program, under ASan+UBSan: an error or a
  parse, never a crash or a hang; deep nesting (`((((`, `{{{{`, 100 000 levels) must
  hit the depth limit.
- **Performance smoke**: 1 M lines through `awk '{s+=$1} END{print s}'`, `/re/` on each
  line, `gsub`; compare with the system awk (expect the same order of magnitude).
- **WASI**: `tests/builtin-awk.sh` under the Node harness (`doc/wasm.md`), with the
  process-needing groups skipped and listed.

### Risks and open questions

- **Float determinism**: `%g` output and `strtod` come from libc; identical across
  glibc/musl/dietlibc for correctly rounded conversions, but dietlibc's
  `printf("%.17g")` is worth checking once (add to `tests/`).
- **libm size**: `sin cos atan2 exp log sqrt pow` are linked only if the awk builtin
  is on; in a static build measure them (Goal 5's tooling) before deciding
  whether math functions are a separate `ENABLE_AWK_MATH` (integer-only `awk`
  is legal but not POSIX).
- **The `getline` and `print >` grammar** are where hand-written awk parsers go
  wrong; they get parser-corpus cases first (step 4), before any interpreter exists.
- **`/` disambiguation by previous token** is a heuristic that is exactly right for
  POSIX's rule but must treat `)` carefully (`(a) /b/ c` is division, `if (x) /re/`
  is a regex): keep the token before `)` on a small stack of "kind of paren" entries.
- **Uninitialised-as-array** (a variable first used as `a[1]`, or passed to a
  function that indexes it) needs the cell to become an array in the caller; get
  the ownership rules into a test before writing the call code.
- **Bytes vs characters** (Issue 8 says characters): conformant in the POSIX locale
  (bytes); deferred to Goal 7 M-steps, listed in `--help`.
- **Memory**: the statement arena is reset per statement, so a long-running `END`
  loop building a huge string keeps only the variable's own copy; test a 100 MB
  string and a 1 M-element array for time and peak size.
- **Shell integration**: `awk`'s stdin is `fd_in->r` (a shell buffer, so it shares
  position with `read`); output goes through `fd_out->w` and must be flushed before
  any child command and at exit; `exit` inside awk returns a status, it never exits
  the shell; `ENVIRON` changes do not reach child processes (unspecified in POSIX).
- **Scope creep**: GNU `gensub`, `strftime`, `asort`, `PROCINFO`, `--re-interval`,
  `printf %'d`, dynamic `FIELDWIDTHS`, and `-i inplace` are **not** planned.

### How to measure

```sh
# sizes of the pieces, stripped MinSizeRel build
cmake -S . -B /tmp/sedawk -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILTIN_SED=ON -DBUILTIN_AWK=ON
cmake --build /tmp/sedawk -j8
size /tmp/sedawk/CMakeFiles/libshell.dir/src/builtin/builtin_{sed,awk}.c.o
nm --size-sort -S /tmp/sedawk/CMakeFiles/libshell.dir/src/builtin/builtin_awk.c.o | tail -25
strip /tmp/sedawk/shish && stat -c%s /tmp/sedawk/shish       # vs the same build with both OFF
# the libm/printf cost in a static build
cmake -S . -B /tmp/sedawk-static -DLINK_STATIC=ON -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILTIN_AWK=ON
# speed
seq 1 1000000 > /tmp/n; time /tmp/sedawk/shish -c "awk '{s+=\$1} END{print s}' /tmp/n"; time awk '{s+=$1} END{print s}' /tmp/n
```

### Status (2026-09-25): what actually shipped

**Foundation**: `lib/hashmap` (ported from `../c-utils/lib/hashmap`, plus a
new `hashmap_next`/`hashmap_clear` iterator pair not in the original) for
awk arrays; `str_ndup`, `byte_lower`/`byte_upper` (new, small). `arena` and
`dfa_replace`/`dfa_repl` were already done from Goal 3/Goal 6 work and
needed no changes. The printf-formatter-refactor line item was **not**
done — `text/awk/awk_printf.c` is awk's own, independent implementation
(reconstructs a `snprintf` spec from parsed flags/width/precision plus one
fixed conversion letter, never the program's format text verbatim, so
there is no format-string-injection risk despite `CONVFMT`/`OFMT`-style
trust elsewhere) — reusing `builtin_printf.c` would have meant touching the
shell's own default-on `printf` path for a saving that didn't seem worth
that risk once `awk_printf.c` turned out to be ≈250 lines on its own.

**`text/awk/`**: lexer, precedence-ladder parser (one function per
precedence level rather than a single table-driven climber — awk's `$`
tightness, unary-vs-`^`, print's bare-`>` ambiguity, concatenation-with-
no-token and `(i,j) in a` are each a grammar-level exception, and a ladder
keeps each one local to the one function that needs it), tree-walking
interpreter, values/conversions, fields/records (lazy split and lazy `$0`
rebuild, both hooked through every path that can observe them: reading
`$n`, reading `NF`, `NF=`, `$n=`), built-ins (`length substr index split
sub gsub match sprintf sin cos atan2 exp log sqrt int rand srand tolower
toupper system close fflush`), `printf`/`sprintf`, I/O (`getline` in all
four POSIX forms, `>`/`>>`/`|` redirection, `close`/`fflush`), and the
main driver (`BEGIN`/`END`, `ARGV`/`ARGC`/`ENVIRON`, `-v`/operand
assignments, `-F`). `src/builtin/extra/builtin_awk.c` is the shell-facing
client (option parsing, wiring `struct awk_io` to `fd_in`/`fd_out`/
`fd_err` and `lib/open.h`). Registered as `EXTRA_BUILTINS` (off by
default, like `sed`/`grep`); `text/awk/*.c` (the engine) is always
compiled in, same as `text/sed/`.

**Deviations from the design above** (each already anticipated as a risk
or a cut-list item there, not a surprise):

- **`cmd | getline`, `print | cmd`, `system()` are not wired up.** The
  engine's own interface (`struct awk_io.run_shell`) is exactly the hook
  the design calls for, and the interpreter already has full support for
  all three (parses them, dispatches through `run_shell`, gives a clean
  runtime error when it's `NULL`) — `builtin_awk.c` just leaves
  `run_shell` unset. Wiring it to the shell's own evaluator (the
  `$(...)`/`eval` machinery) is a distinct, self-contained follow-up.
- **`RS=""` (paragraph mode) and a regex `RS`** are not implemented; `RS`
  is always treated as a single byte (first character), default `"\n"`.
- **Byte, not character, semantics** for `length`/`substr`/`index`/`match`/
  `printf %c` — conformant in the POSIX locale, same as the rest of the
  shell pending Goal 7.
- **`for (k in a)` order** is hashmap bucket order (unspecified in POSIX,
  as planned).
- Hashmap keys/values are heap-allocated (`alloc()`/`str_ndup`) rather than
  arena-owned, since awk arrays are mutated (`delete`, reassignment) far
  more than the write-once, reset-in-bulk shape an arena suits; expression
  *temporaries* do use `st->tmp`, reset once per statement, as planned.

None of the above went into `BUGS`: the `run_shell`/`RS=""` cut is already
a documented omission in `help_awk` (`src/builtin/extra/builtin_awk.c`),
byte-vs-character semantics is a standing repo-wide condition with no
precedent of its own `BUGS` entry on any other builtin, `for-in` order is
POSIX-unspecified so not a discrepancy, and regex `RS` is a gawk extension
POSIX never required.

**Testing**: a standalone build against a POSIX-`read`/`write`-backed
`struct awk_io` (not wired into the shell) was run under
`-fsanitize=address,undefined` across ~30 programs spanning every
built-in, arrays, recursion, field/`NF` assignment and rebuild, `getline`,
and `printf` — zero leaks, zero sanitizer reports. Full `ctest` (both the
default builtin set and `-DENABLE_ALL_BUILTINS=ON`, each with
`-DBUILTIN_AWK=ON`) shows the identical pre-existing 52/157 failure set
(all signal/job-control `posix/*-p.tst` cases plus `fixed.sh`, none of
them awk-related) — no regressions. `tests/builtin-awk.sh` (new, gated by
`BUILTIN_AWK`) covers fields, `NF`/`NR`, string/math built-ins, `printf`,
arrays (`for`/`in`/`delete`), user functions (recursion, array-by-
reference), control flow, and the numeric-string comparison rules.

---

## Goal 12 (secondary) — more `EXTRA_BUILTINS`: POSIX utilities real scripts call most, still external

**Not started; this section is the candidate list.** `grep` and `sed` (Goal 6,
Goal 11) already moved from "external" to "builtin"; with those two done, the
next-highest-value external commands were picked by counting real usage rather
than guessing. Same rationale as Goal 10 (`cp`/`mv`): each is free where an
external binary already exists (a builtin only wins the `PATH` lookup +
`fork`+`exec`) and is a real capability where none does (WASI, a from-scratch
container, `-DLINK_STATIC=ON` single-file image).

**Evidence.** A histogram of every command word bash actually dispatched
(builtin/external/keyword/notfound) across a large corpus of real-world shell
scripts (`../plot-cv/shell-commands-histogram.txt`, 3545 distinct names),
filtered to names that are also POSIX.1-2024 utilities
(<https://pubs.opengroup.org/onlinepubs/9799919799/utilities/>) and not
already a shish builtin:

| Count | Utility | Count | Utility | Count | Utility |
|---|---|---|---|---|---|
| 2747 | `cp` | 400 | `uniq` | 26 | `od` |
| 1772 | `mv` | 267 | `getconf` | 20 | `df` |
| 1326 | `sort` | 264 | `xargs` | 19 | `comm` |
| 927 | `tr` | 239 | `tail` | 12 | `paste` |
| 794 | `diff` | 228 | `cmp` | 12 | `nohup` |
| 551 | `date` | 192 | `id` | 10 | `nice` |
| 526 | `cut` | 109 | `env` | 10 | `fold` |
| 484 | `head` | 75 | `bc` | 8 | `tty` |
| 412 | `tput` | 73 | `dd` | 6 | `mkfifo` |
|  |  | 37 | `chown` | 5 | `join` |
|  |  | 29 | `du` | 4 | `expand` |

(`seq`, `install`, `dir`, `yes`, `stat`, `groups`, `arch`, `stty`, `sum`,
`sync`, `nproc`, `truncate`, `fmt`, `base64`, `mknod` also appear at similar
frequencies but are **not** POSIX utilities — GNU/BSD-only or shell-builtin
duplicates — so they stay out of scope per the "design spec is POSIX" rule at
the top of this file. `awk` (1065) is already Goal 11.)

**Not a flat priority order.** Group by what they'd cost and who needs them:

- **`cp`/`mv`** already have a plan (Goal 10) — by far the two highest counts,
  do those first.
- **Cheap, self-contained, high count**: `tr`, `cut`, `head`, `tail`, `uniq`,
  `date`, `sort` (the last needs a comparison/key-field engine — `-k`, `-t`,
  `-n`, `-r`, merge sort over lines held in one arena — closer in size to
  `builtin_sed.c` than to `builtin_wc.c`). These cover the bulk of "text
  pipeline" idioms (`... | sort | uniq -c`, `cut -d: -f1`, `date +%s`) that
  currently force a `fork`+`exec` even in a script that otherwise runs
  builtins-only, and are the ones that matter most for the standalone/WASI/
  no-`PATH` goal, since scripts reach for them constantly and unconditionally.
- **`diff`**: real value (794 uses) but by far the biggest of this batch —
  a full line-diff needs an LCS/Myers engine, closer in scope to `text/dfa`
  than to a single `builtin_*.c`; would want its own `text/diff/` the way
  `sed` got `text/sed/`. Worth a Goal of its own if picked up.
- **System/identity utilities**: `id`, `env`, `chown`, `du`, `df`, `dd`,
  `nice`, `nohup`, `tty`, `logname`, `who` — each individually small (most
  are one syscall plus formatting) but only pay off where `fork`+`exec` of
  the real one is unavailable; lower priority than the text-pipeline group
  since they're less central to typical scripts.
- **Niche/legacy POSIX text utilities**: `getconf`, `cmp`, `bc`,
  `comm`, `paste`, `fold`, `mkfifo`, `join`, `expand`/`unexpand`, `od`,
  `pr`, `cksum`, `tsort`, `csplit`, `pathchk`, `split`, `chgrp` — real
  POSIX utilities, all seen in the corpus, but each at low enough frequency
  (and `bc` non-trivial in scope) that they're candidates, not a
  near-term plan; no per-utility sizing has been done for these yet.
  (`xargs` left this list: it is now an `EXTRA_BUILTIN`,
  `src/builtin/extra/builtin_xargs.c`, with `-0 -a -d -I -L -l -n -o -P -p -r -t`;
  gaps: no blank/quote splitting of input lines, `-E`/`-s`/`-x` accepted but
  ignored, empty input with a utility given runs nothing. Tests:
  `tests/builtin-xargs.sh`.)

No file layout, option sets, or size estimates have been worked out for any
of these yet — that's the next step once one is picked up, following the
Goal 6/10/11 template (POSIX page -> option table -> LOC estimate -> `BUGS`
entries for any deliberately-omitted option).

---

## Goal 13 (secondary) — pull-based filter chaining for pure-builtin pipeline segments

**MVP done (2026-09-25).** Motivating case: `grep <in.txt 'pat' | sed
's/x/y/'` — a common idiom (select lines, then edit them) where both stages
are already shish builtins. Before this, every non-last stage paid a real
`fork()`/`pipe()` even though nothing about a builtin actually needs one;
the last stage already avoided it via lastpipe (see the pipeline discussion
this goal grew out of, 2026-09-25). `eval_pipeline()` now lets a pipeline's
*entire* non-last prefix (not just its first stage) chain straight into the
true last stage through the in-process buffers below, all-or-nothing: every
non-last stage has to be a filter-capable builtin invoked with literal
argv (`pipeline_filter_prepare_chain()`) and actually `.open()` (a runtime
decline, e.g. `grep -c`/`-q`, rolls the whole chain back), or the pipeline
runs exactly as it did before this goal. `cat <file> | grep -E '(a|b)' |
sed '...'` now runs with zero `fork()`/`pipe()` calls end to end, confirmed
under `strace -f -e trace=clone`. Resolves open question 3, below.
Still open: `sed_step`'s resumability was never the risk it looked like
(sed's existing filter mode already reads incrementally); open question 4
(`!HAVE_FORK` builds) is unaddressed — `eval_pipeline_sequential()` doesn't
attempt chaining yet.

### The constraint that shapes the whole design: `sed` and `grep` are optional

Both are `EXTRA_BUILTINS` (`cmake/Builtins.cmake`), individually toggleable
(`-DBUILTIN_SED=OFF` / `-DBUILTIN_GREP=OFF`, or simply absent from a
`MINIMAL_BUILTINS`-only build), and every build without a working `fork()`
already runs pipelines through `eval_pipeline_sequential()` instead. Two
requirements follow, non-negotiable for any implementation of this goal:

1. **Compiles clean with either or both builtins disabled.** No code
   outside `src/builtin/extra/builtin_{sed,grep}.c` may name a
   sed/grep-specific type (`struct sed*`, `text/sed.h`, grep's internals).
   Generic pipeline/fdtable code gates on availability the same way
   `#if BUILTIN_TRAP` already does throughout `src/`
   (`eval_pipeline.c`, `job_wait.c`, `sh_loop.c`, `expand_command.c`,
   `eval_subshell.c`, ...) — this goal adds `#if BUILTIN_SED` /
   `#if BUILTIN_GREP` (from the already-generated `builtin_config.h`) as
   the same kind of guard, nothing novel.
2. **Every pipeline still runs correctly via plain `fork()`+`pipe()`,
   unconditionally.** `H_PROGRAM` (any external command, including a real
   `/usr/bin/sed` or `/usr/bin/grep` when the builtin is disabled or a
   pipeline just isn't the shape this goal targets) already always forks
   (`exec_command.c`'s `H_PROGRAM` comment) — this goal never touches that
   path. Chaining is strictly additive and opt-in per pipeline shape: any
   pipeline that doesn't match its narrow preconditions falls straight
   back to today's `job_fork()`/`fd_pipe()`, unchanged. There is no new
   failure mode here — absence of chaining is always safe, it's just the
   status quo.

### Design principle that follows from (1): no builtin-specific code above the builtin layer

The fdtable/`eval_pipeline` layer can only depend on a generic,
builtin-agnostic interface — never on `grep_step`/`sed_step` by name:

```c
/* one steppable builtin's filter interface -- lives in the builtin's own
 * extra/builtin_*.c; eval_pipeline never has to know the builtin's name
 * to call it. */
struct filter_ops {
  /* NULL return: this invocation isn't streamable (flags outside the
   * streamable subset, e.g. grep -c/-q; bad pattern; ...) -- caller
   * falls back to fork()+pipe(), same as if .filter were NULL at all. */
  void* (*open)(int argc, char** argv, buffer* upstream);
  ssize_t (*read)(void* ctx, char* buf, size_t len); /* buffer_op_proto-compatible */
  int (*status)(void* ctx);                          /* valid once read() returns EOF */
  void (*close)(void* ctx);
};
```

`struct builtin_cmd` (`builtin_table.c`) gets an optional
`const struct filter_ops* filter;` member — `NULL` for every builtin that
isn't steppable (everything except `sed`/`grep` to start). `builtin_table.c`
is already generated per-name from `builtin_config.h`'s `#if`s, so an
entry whose `.filter` would reference `grep_filter_ops` sits inside
`#if BUILTIN_GREP` and costs nothing when that's off. `eval_pipeline()`
itself never says `"sed"`/`"grep"` anywhere — it only ever reads
`cmd->builtin->filter`, generic across every current and future steppable
builtin (a later `awk` slots in the same way, no new pipeline-layer code).

### Pieces

1. `struct filter_ops` + the `.filter` registration above. Always
   compiled (empty/unused if nothing implements it); ≈ 30 lines, no
   builtin-specific logic.
2. `grep_step()`/`grep_ctx` — refactor `builtin_grep.c`'s existing
   per-line loop into a step function reused by both standalone
   `builtin_grep()` (loop it to EOF, identical output to today) and the
   filter path. `#if BUILTIN_GREP` only. No cross-record state, so this
   is close to a pure refactor.
3. `sed_step()` — genuinely new engine work, not a refactor: `sed_run()`
   today loops to completion (`sed_cycle.c`). A resumable step needs that
   outer loop restructured to return once it has produced output (or
   needs more input than upstream currently has) and pick back up from
   the same point next call, including mid-`N` state and the pending
   `a`/`r` append queue's flush timing. `#if BUILTIN_SED` only. This is
   the one piece of the whole goal that isn't glue.
4. `FD_FILTER` fd mode (`src/fd.h`, alongside `FD_HERE`/`FD_SUBST`): a
   `struct fd` whose `r` buffer's `op` drives a `struct filter_ops`
   instance through its `cookie`. Builtin-agnostic — references only the
   vtable, always compiled.
5. `fdtable_exec()` gains an `FD_FILTER` case, materializing it into a
   temp file the same way `fdtable_here()` already does for heredocs
   (`src/fdtable/fdtable_here.c`): drain to completion, write to a temp
   file, swap in `buffer_op_read`. Reached whenever a real fd becomes
   necessary — an external program downstream, or an explicit fd
   capture. Builtin-agnostic, always compiled.
6. `eval_pipeline()` detection: adjacent `H_BUILTIN` stages where
   `cmd->builtin->filter != NULL` on every non-last stage skip
   `job_fork()`/`fd_pipe()` for those stages, calling `.open()` instead
   (which decides per actual argv whether *this* invocation streams) and
   wiring the result as the next stage's `fd_in`. The last stage is
   untouched — runs exactly as today's lastpipe branch (forked or not),
   with its stdin possibly now an `FD_FILTER` fd instead of a pipe. A
   `NULL` from `.open()` (unstreamable args, no `.filter` at all, or the
   builtin simply not compiled in) means that stage falls straight back
   to `job_fork()`/`fd_pipe()` — always compiled, but its actual
   behavior collapses to "always fork" the moment neither `BUILTIN_SED`
   nor `BUILTIN_GREP` is on, with no dead branches left dangling.

### Exit status

Every steppable builtin still computes its real exit status via the same
logic it always has (`grep_ctx.status`, finalized when `read()` returns
EOF) — filter mode changes *when* that status becomes known, never *what*
it means:

- **Chained stage is not last** (`grep | sed`): its status doesn't drive
  `$?` today regardless (POSIX: last stage only), and shish has no
  `pipefail`/`PIPESTATUS` yet to consume it
  (`BUGS: set-pipefail-missing-and-noclobber-not-enforced`) — but since
  the last stage normally drains its whole input, the upstream filter's
  status will be correctly finalized as a side effect anyway, ready for
  whenever `pipefail` lands, at no extra cost now.
- **Chained stage's consumer stops early** (the last stage's own `q`/`Q`,
  or a future `head`) before the filter reaches real EOF: its status is
  never finalized. Same ambiguity a real forked pipe already has today
  (early close -> SIGPIPE -> a discarded, signal-shaped exit status) —
  leave it an explicit "unset" rather than inventing a synthetic value;
  revisit only once `pipefail` needs a real answer.
- **Chained stage becomes the pipeline's *last* stage instead**
  (`sed | grep`): already works today, unmodified — lastpipe runs it to
  completion in-process exactly as now. This goal adds nothing and
  changes nothing on that path.

### Sizing (rough; no code written yet)

| Piece | Lines | Gated on |
|---|---|---|
| `struct filter_ops` + registration | ≈ 30 | always compiled |
| `grep_step`/`grep_ctx` refactor | ≈ 60 (mostly moved, not new) | `BUILTIN_GREP` |
| `sed_step` (`sed_cycle.c` restructure) | ≈ 150-200 (real new logic) | `BUILTIN_SED` |
| `FD_FILTER` fd mode | ≈ 40 | always compiled |
| `fdtable_exec()` materialization case | ≈ 50 (mostly reused from `fdtable_here`) | always compiled |
| `eval_pipeline()` detection + wiring | ≈ 80 | always compiled |
| **Total** | **≈ 410-460** | |

### Open questions, ordered by how much they'd change the shape above

1. **Does `.open()`'s per-call opt-out (grep `-c`/`-q`) belong in
   `struct filter_ops`, or should those flags just never register
   `.filter` at all and always fork?** Leaning toward per-call — it's
   the same mechanism that already has to handle "not compiled in" and
   "wrong builtin" uniformly, so handling "wrong flags for this specific
   invocation" costs nothing extra.
2. **`sed_step`'s resumability is the one piece of unproven engine
   work.** Worth a small standalone spike — can `sed_cycle.c`'s loop be
   restructured to pause/resume cleanly around `N` and the append-queue's
   flush timing? — before committing to the rest of the design around it.
3. **Resolved (2026-09-25): three-or-more-stage chains** (`cat | grep |
   sed`) needed no new design beyond scanning the pipeline's whole
   non-last prefix at once instead of just its first stage
   (`pipeline_filter_prepare_chain()`) — the concern this item raised
   (a middle stage's `.read()` needing to cleanly signal
   need-more-input) never came up because the chain is pull-based:
   the true last stage's own `.read()` calls cascade backwards through
   `chain_link[]` on demand, so an earlier stage is never asked to
   produce output before its consumer wants it.
4. **`!HAVE_FORK`/WASI builds get correctness, not just speed, from
   this.** `eval_pipeline_sequential()`'s existing per-stage
   full-materialization fallback (`eval_pipeline.c:29-48`) hangs on an
   infinite producer (`yes | sed ... | head` never finishes stage one).
   A `FD_FILTER`-chained all-builtin pipeline wouldn't need that
   fallback at all for its own stretch, since no `fork()`/`pipe()` was
   ever required there either way. Worth deciding whether
   `eval_pipeline_sequential()` should attempt filter-chaining first and
   only fall back to full materialization once the chain isn't entirely
   steppable builtins.

---

## Also open (secondary)

- **WASI build (`cfg-wasi`, `doc/wasm.md`) — builds and runs (Node, webassembly.sh).** `build/wasi/shish` is 248 KB and runs under Node's WASI
  (`--experimental-wasm-exnref`): 18 of 34 `tests/*.sh` pass unmodified,
  the rest need fork/external commands or job control. Open:
  - **webassembly.sh works** (Chrome 152, upload via `wapm upload`,
    2026-09-19). The module needs WebAssembly exception handling
    (`exnref`; wasi-sdk 34 no longer emits the legacy opcodes); an older
    browser lacking it cannot load the module — the fallback would be a
    build without EH, i.e. replacing the `setjmp` unwinding in
    `src/eval/` (design-sized). The interactive prompt there is untried.
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
  `complete`/`compgen`) is Goal 9. UTF-8-aware editing (per-character cursor and
  backspace, display columns) is milestone M5 of Goal 7.

---

## Goal 14 (secondary) — evaluator trace (`SHISH_TRACE`): finish the instrumentation

**Steps 1-3 and the start-up trace are done (2026-09-26); the design, the event
catalogue and the rest of the plan are in [`doc/debug-output.md`](doc/debug-output.md).**
A uniform, parseable trace layer (`src/trace.h`, `src/trace/`) replaced the old
`debug.log` prints: one event per line, `[pid:depth] mod.event(k=v, ...)`, one
`write()` each, selected at run time with `SHISH_TRACE=exec,fd,...|all|-name` and
`SHISH_TRACE_FILE=path|-` (default `trace.log`), compiled out without
`DEBUG_OUTPUT`. Modules with events today: `exec`, `builtin`, `fd`, `fdstack`,
`fdtable`, `eval`, `expand`, `redir`, `parse`, `sh` (start-up: `sh.start/input/mode`),
`sig` (traps only). `debug.log` is no longer created.

**Still open**

- **Steps 4-6 are done** (2026-09-26): fd\*/fdstack/fdtable entry events, var/sh/job events,
  and `tools/trace2seq` (process tree, `-c EVENT` counts; test: `tests/trace2seq.sh`).
  Gaps closed 2026-09-27: `sig.block/unblock/blocknone`, `sh.opt`, `var.create`, `sh.getcwd`,
  `sig.handler`/`sig.trap.*` (queued from signal context in `src/trace/trace_defer.c`, printed by
  `trace_flush()`), `fdtable.lazy/wish/gap/open/openfd/close` results, `trace2seq -s` (Mermaid).
  Also done: `job.fork` for an external `cmd &`, `job.signal` (queued), `job.table`, `sh.pushargs/popargs`.
  Not traced on purpose: `var.import` (one line per environment variable; `var.export` reports the count).
- **`SHISH_TRACE` is read from the process environment once**, at the first event; an
  `export SHISH_TRACE=...` inside a running script is not seen. Reading it through `var_get`
  would fix that but touches every event's startup path.
- **Autotools:** works in-tree only (`./autogen.sh && ./configure --enable-debug CPPFLAGS=...`,
  serial `make`, then `./config.status src/builtin_config.h` once — configure does not run its
  `AC_CONFIG_COMMANDS` step, cause not found). `src/*/Makefile.in` `MODULES` lists are
  hand-maintained and drift (`expand_getorcreate`, `fd_filter`, `trace_fd` had to be fixed by hand).
- **`xargs` and `timeout` run their command through `exec_command()`** (no private fork/execvp):
  builtins, functions and programs all work and "$(...)" capture comes from `exec_program()`.
  `timeout` kills via a SIGALRM handler aimed at `exec_child_pid`; a builtin with no program of
  the same name, or a function, is forked (`X_NOWAIT`) so it can be killed, and its output is
  then not captured in "$(...)". "BUGS" still has the "redirect inside `$(...)` is ignored for
  forked commands" entry (root cause: `fdstack_pipe()` overrides whatever fd 1 was redirected to).
- **What the trace already found:** the shell's internal pipe (fds 128/129) leaks into every
  exec'd program (`fdtable.exec.fds` shows it); `cmdsubst_ran` was cleared after word expansion
  (`fixes/242`); `xargs`'s uninitialised `items.c`, `-d` separator not stripped, output escaping
  `$(...)` (`fixes/240`, `fixes/241`).


---

## Goal 15 (secondary) — vi mode (`src/term/term_vimode.c`): gaps against vim/POSIX `set -o vi`

Implemented: ESC command mode; motions `h l 0 ^ $ w b e W B E f F t T ; ,` with counts;
operators `d c y` (+ `dd cc yy`, `cw` = `ce`); `x X D C s S r p P i a I A k j /`. Tests:
`tests/term-vi.sh`. `vi` is always on: there is no `set -o vi` / `set -o emacs` switch.

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
- **Counts:** `0` is a motion only when not part of a count (done); missing are counts for `i a I A` (`3ix<ESC>` repeats), for `p` / `P` (`3p`), `.` and `~`.
- **Repeat find:** `;` / `,` work, but `t` repeated with `;` does not skip past the adjacent character the way vim does when `cpo` has no `;`.
- **Pending-state display:** no `-- INSERT --` / `-- NORMAL --` indicator, no cursor-shape change (`\e[2 q` block / `\e[6 q` bar), no display of a pending operator or count.
- **Escape handling:** a lone ESC waits 50 ms for a following key (fixed timeout, not `ttimeoutlen`); there is no way to configure it.
- **Multi-line entries:** history entries with embedded newlines are shown on one line; there is no line-wise `j` / `k` / `o` / `dd` inside them.
- **Wide characters:** columns are bytes, so `l` / `x` / `w` step through a UTF-8 sequence one byte at a time.
- **Options:** none of `set -o vi`, `EDITRC` / `inputrc`-style key rebinding, `bind`, or a `vi`-mode `KEYTIMEOUT` is honoured.
