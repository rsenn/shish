# TODO / Roadmap

Leverage-sorted list of what's still open. Fixed work lives in `git log` and
`fixes/*.patch`, not here — this file only tracks what's left to do. See
`BUGS` for confirmed, reproducible defects with repro steps.

---

## Goal 1 — POSIX conformance

`./configure` (this project's own stress test — autoconf output exercises
nested command substitution, heavy fd juggling, `eval`, here-docs,
trap/exit interplay) now runs to completion end-to-end.

A Termux (Android/bionic) user report of `shish configure` segfaulting
(2026-08-04) led to setting up a `-fsanitize=address,undefined` build
(`cfg()` from `cfg-cmake.sh`, e.g. `builddir=build/x86_64-linux-gnu-asan
CC=clang CXX=clang++ cfg -DCMAKE_C_FLAGS="-fsanitize=address,undefined
-g -O0" ...`) — the crash reproduced identically under Linux/glibc, so
it was never bionic-specific, just easier to hit there. Root cause:
`lib/path/path_fnmatch.c`'s `'*'`-matching recursed once per character
of the string being matched (searching for a split point), so any
single `*` in a glob/case pattern matched against a long enough string
(exactly what autoconf-generated scripts do, e.g. `case
$ac_configure_args in *\'*)`) blew the stack. Fixed (`fixes/126`) by
rewriting the function to be fully iterative — a single "most recently
seen `*`" backtrack bookmark, consulted whenever a match attempt
fails, replaces all recursion (the standard technique for wildcard
matching), so match cost depends on neither the string's length nor
the pattern's `*` count. Chasing the same sanitizer
run turned up four more real, independent bugs along the way (none
related to the crash itself, all found because the ASan/UBSan build
let `configure` run much further than a plain build's silent-UB
tolerance had ever surfaced before): a shift-by-64 in
`var_rndhash()`'s rotate macros, two "form a member address through a
null pointer" bugs (`exec_search()`'s empty-function-list walk,
`redir_source()`'s here-doc-list walk), and a `memcpy()`-with-null-src
issue at two `stralloc` call sites when copying an empty/unset buffer
— fixed as `fixes/123`-`125`, `127` respectively. A later, separate
sanitizer finding from the same investigation initially looked like
more of the same "real but pervasive, not worth fixing" kind, but
turned out to be the actual Termux crash's own root cause once pushed
on further: `src/tree.h`'s `__packed` macro (deliberately a no-op —
the real `__attribute__((packed))` is commented out) collides by name
with Android Bionic's `<sys/cdefs.h>`, which already defines `__packed`
for real — silently, unintentionally packing every `union node` field
on that platform only, corrupting reads since different node kinds
don't all pack to the same offsets. Fixed (`fixes/131`) by renaming
shish's own macro to `SHISH_TREE_PACKED` (now fixed, no longer listed
in `BUGS`). `BUGS:
ubsan-buffer-op-proto-function-type-mismatch` from the same sweep is
still a real, deliberate, not-worth-fixing tradeoff, unrelated to this
one. The `tests/posix`
conformance suite (120 files) is wired into `ctest` and runs by default.
`tests/yash` (119 more files) is wired in the same way but gated behind
its own `-DDO_YASH_TESTS=ON` (off by default — several files hang and
only terminate via their own 120s `TIMEOUT`, `BUGS:
yash-suite-other-hangs`, which otherwise dominates a default `ctest`
run's wall time).

A separate, much larger real-world stress test — gettext-tools'
`configure` (autoconf-generated, gnulib-heavy, ~30k lines) — now also
runs to completion (~2.5 min, no hang/crash/core dump) as of 2026-07-30,
after fixing three bugs found chasing a reported hang in `sh_forked.c`'s
`for(sh = sh->parent; sh; sh = next)` loop: `eval_exit()` ("exit" inside
a function/subshell) had the same commented-out fdstack/varstack/source
unwind bug already fixed for `eval_return`/`eval_jump` (`fixes/97`,
`fixes/99`), plus its own extra wrinkle — it deliberately walks past any
number of `E_FUNCTION` frames to reach the nearest subshell/root, so it
also had to pop each skipped function call's `sh_push()`ed `struct env`,
or that env dangles and corrupts `sh->parent` the moment its stack slot
is reused (`fixes/101`). A second, unrelated bug in the same fix: a bogus
`if(e == sh->parent->eval) return` early-out silently turned `exit` into
a no-op whenever it was called two or more function calls deep in the
same process. Chasing the crash further surfaced a third, independent
bug in `eval_function()`, which stole (moved, then nulled) a function
definition's name/body pointers out of its AST node — safe only if that
node is evaluated once, so it segfaulted the moment the same node was
evaluated again (a function defined inside a loop, or inside one of
shish's in-process `(...)` subshells); fixed via a new `tree_copy()`
helper (`fixes/102`). The script now stops instead on a real
`configure`-level detection failure (`socklen_t`), which traces back to
`BUGS: confdefs-h-duplication`, still open.

Separately, reported and fixed the same day: Ctrl-C at the controlling
terminal not reliably stopping a running `./configure`, "only after many
presses". Root cause was `exec_program.c`/`job_fork.c` unconditionally
`setpgid()`-ing every external command (single, pipeline member,
foreground or background) into its own process group regardless of
whether job control was actually active — real bash never does this for
a non-interactive script, and confirmed by direct repro: a terminal
SIGINT killed shish itself but left the external command it had just
started (a real `gcc` invocation, mid-compile, during an actual
gettext-tools `configure` run) orphaned and running. Fixed (`fixes/103`)
by gating all of this behind `sh->opts.monitor`, plus making
`job_wait()` fall back to waiting for any child instead of just a
pipeline's first member when a job has no real process group of its own.
Chasing this also turned up a related, independent bug — `job_terminal`
was always -1 (terminal handoff to a running job/pipeline never happened
at all, interactive or not) due to an init-order bug (`job_init()` ran
before `term_init()` ever set `FD_TERM`) — fixed the same day
(`fixes/104`), restoring real interactive job control (`fg`, resuming a
Ctrl-Z-stopped job).

Despite both fixes, Ctrl-C against a real `./configure` run still wasn't
fully reliable — root-caused and fixed the same day (`fixes/105`).
autoconf-generated scripts (including gettext-tools') install a real
`trap ... INT` for cleanup, and shish's `trap_handler()` used to run the
entire trap body — allocation-heavy `eval_tree()`, potentially including
`exit` — directly from asynchronous signal-handler context, the same
category of bug already fixed for `SIGCHLD`'s own handler (`fixes/87`)
but never applied to user traps. That alone had two consequences, both
fixed together: (1) an `exit` inside such a trap only ever unwound to
the nearest in-process subshell boundary rather than actually
terminating the process (correct for an *ordinary* synchronous "exit"
inside a script's own `(...)`, wrong for one triggered asynchronously by
a signal — shish's subshells don't fork, so "just this subshell" means
the rest of the script keeps running); (2) whatever syscall the
interrupted handler was in the middle of (typically `job_wait()`'s
blocking `wait_pid()`) would just transparently resume once the unsafe
handler returned, via `SA_RESTART` — so a trap that itself called `exit`
often never got a real chance to run until ordinary control flow reached
a dispatch point on its own, if ever.

Fixed with the same self-pipe/deferred-dispatch redesign `fixes/87` gave
`sh_onsig()`: the real OS signal handler (`trap_relay()`) now only does
async-signal-safe work (record which signal fired, wake a self-pipe),
with the actual trap body evaluation (`trap_run_pending()`) deferred to
ordinary context — `sh_loop()`'s main loop, `term_read()`'s `select()`
wakeup, and (critically, since this is where a real Ctrl-C typically
lands) `job_wait()`'s own retry loop, checked both before *and* after
each `wait_pid()` call so it fires whether the signal arrived before or
during the block. `sig_push()` now installs with the new `SA_NORESTART`
flag (`lib/sig.h`) so the interrupted syscall actually returns instead
of silently resuming. A new `sh_async_exit` flag (`sh.h`) marks an
`exit` triggered this way so `eval_subshell.c` propagates through every
enclosing in-process subshell level instead of stopping at the first
one, ultimately reaching real process termination.

Deferring dispatch also surfaced its own real race, fixed in the same
change: a signal can be delivered (and recorded as pending) before it's
drained, e.g. while inside a subshell that's exiting and, as part of its
own normal cleanup, uninstalling the very trap that signal was meant to
fire — left set, the next dispatch ran against whatever's now installed
for that signal (nothing), and `trap_handler()`'s "no trap found"
fallback silently killed the whole shell via `sh_exit(1)`. Both places
that uninstall a real-signal trap (`trap_uninstall()`,
`trap_snapshot_restore()`) now also discard any not-yet-dispatched
pending occurrence of it.

Verified against the real gettext-tools `configure` script: a real
SIGINT sent while blocked on an actual `gcc` invocation (matching
autoconf's own `trap ... INT` boilerplate, itself inside the
`{ (eval "$ac_link") ...; }` idiom autoconf wraps every compile probe
in) now reliably kills both `gcc` and shish, with no leftover processes,
across repeated trials at different points in the run.

`BUGS: confdefs-h-duplication` — the one thing still stopping the real
gettext-tools `configure` from running to genuine completion — turned out
to already be fixed, as an unplanned side effect of `fixes/109` (below).
Re-running the exact same script after that fix landed: it now runs all
the way through, including the nested `examples/configure` sub-run,
producing a complete `config.h`/`config.status`/`Makefile` with no
duplicate `#define`s and no "undefined reference to main" — the
`confdefs.h`-corruption symptoms this entry was tracking are gone.
Whatever intermediate state `$(...)` expressions used throughout
`confdefs.h`-adjacent code (`ac_ext` computation, etc.) were computing
was getting corrupted by exactly the state-leak `fixes/109` fixed; never
independently root-caused beyond that, but the script's own completion
is about as strong a confirmation as this gets.

Also fixed the same day, on request, three smaller and one bonus bug,
each with its own regression test in `tests/fixed.sh`:

- `BUGS: ln-trailing-slash-on-plain-destination` (`fixes/106`) —
  `builtin_ln()` unconditionally appended a `/` to the destination
  before ever checking whether it's a directory, so `ln -s target name`
  (the common case) always failed with ENOTDIR. Also rejects (rather
  than silently mis-linking only the last one) more than one source
  without an existing directory destination.
- `BUGS: set-errexit-not-enforced` (`fixes/107`) — `set -e`'s bit was
  tracked but nothing read it. Enforced in both places a sequential
  command list is actually walked (`eval_tree.c`'s own per-node loop,
  and `eval_cmdlist.c`'s separate one for `;`/newline-separated
  commands sharing one `N_LIST` node — a real gap: `"set -e; false;
  echo bad"` all on one line went through only the latter), each
  calling `sh_exit()` the same way an explicit `exit` would.

  Implementing POSIX's specific exemptions correctly took three
  iterations, validated the whole way against `tests/posix/errexit-p.tst`
  (yash's own errexit conformance suite, 53 cases — not wired into the
  default `ctest` run, gated the same as the rest of `tests/posix` — run
  manually via `sh tests/run-tst.sh <absolute-path-to-shish>
  tests/posix errexit-p.tst`; a *relative* testee path breaks since the
  harness `cd`s into the test dir first, same footgun as `fixes/89`'s
  own investigation hit): first pass (exempt an AND-OR list's non-last
  operand, a `!`-negated command, an if/while/until condition) got
  7/53. The exemption turned out to need to propagate much further than
  one node deep — confirmed directly against real bash's actual
  behavior, not just POSIX's text: (1) a 3+-operand chain like `"false
  || false || true"` must not trip on its un-exempted *middle* operand,
  only the chain's true last one matters; (2) the exemption survives
  into a function call or subshell reached while evaluating an exempt
  expression (`"f() { false; }; f && true"` doesn't abort inside `f`);
  (3) it also survives being wrapped in any construct that doesn't
  produce its own independent, opaque exit status — grouping,
  if/elif/else, for, case, while/until bodies — but *not* a subshell,
  whose own returned status is independent and must still be checked
  normally at the outer level (`"{ false && true; }; echo x"` prints
  `x`, but `"( false && true ); echo x"` does not — confirmed directly
  against bash). Implemented via a single global `errexit_suppress`
  counter (`eval.h`) incremented/decremented around exactly the
  durations POSIX exempts, rather than a per-`struct eval` flag — a
  flag scoped to one eval frame can't express point (2), since a
  function call gets a brand-new one. Final result: 49/53. The
  remaining 4 are unrelated, separately-filed bugs (below), not gaps in
  this fix.

  Also found and fixed in the same triage pass, real but unrelated to
  errexit itself: `test`/`[`'s `-ne` (numeric not-equal) and `-nt`
  (file mtime newer-than) share the same second character (`n`), and
  `builtin_test.c`'s binary-operator dispatch only checked that one
  character to decide "this is a file-mtime comparison" — so `test 1
  -ne 2` silently ran as a file-mtime comparison between two
  (nonexistent) files named `1` and `2` instead of the numeric
  comparison it's supposed to be (`fixes/110`; this is what was
  actually breaking `errexit-p.tst`'s own `for`-loop-body case, not the
  errexit logic itself — `test $i -ne 2` inside the loop was always
  evaluating wrong). Now also checks the third character (`t` vs `e`),
  which is what actually distinguishes them.

  Two genuinely separate, real bugs surfaced by the same conformance
  run were found and fixed the same day too, bringing `errexit-p.tst`
  to a clean 53/53 (and improving, never regressing, every other
  `tests/posix/*.tst` file spot-checked against the same fix):

  - `grouping-piped-loses-output-after-internal-failure` (`fixes/111`)
    — `{ a; false; b; } | cat` lost `b`'s output entirely, and turned
    out to have nothing to do with the internal failure (reproduced
    identically with a `true` in its place) or `set -e` (reproduces
    with it off, and pre-existing on `924b1f0e` before any of this
    day's other fixes). Root cause: `eval_pipeline.c` forks each
    pipeline stage and sets `E_EXIT` on the shared `e->flags` to tell
    the *last* command in that stage to `exec()` directly instead of
    returning — `eval_tree.c`'s own per-node loop correctly restricts
    that to just the last node of whatever it's walking, but a `{...}`
    grouping (or a bare `;`-separated `N_LIST`) used as a pipeline
    stage dispatches straight to `eval_cmdlist()` instead, which never
    touched `e->flags`'s `E_EXIT` bit at all — so it stayed set,
    inherited from the fork, for *every* member of the group's body,
    not just its last one. The group's first member got treated as the
    tail call: it ran, then the forked pipeline stage exited
    immediately. `eval_cmdlist()` now scopes `E_EXIT` to its own last
    member, matching `eval_tree()`.
  - `redirect-failure-does-not-block-execution-or-set-status`
    (`fixes/112`) — a failing redirection on a simple command didn't
    stop it from running or affect its reported exit status, traced
    back to two separate gaps in the fdtable's lazy redirection
    resolution (already flagged as a known issue source elsewhere in
    this file): `exec_command.c` resolves a builtin's pending fd
    0/1/2 redirection right before running it (the real `open()` is
    deferred that far) but never checked whether that resolution
    actually succeeded, so it ran the builtin regardless; and a bare
    redirection with no command at all (`<_no_such_file_` alone) never
    got resolved at all, since nothing beyond `exec_command.c` (which
    that case never reaches) forces it — `eval_simple_command.c` now
    forces immediate, not lazy, resolution specifically when there's
    no command to hand the pending fd off to.
- `BUGS: squoted-backslash-newline-swallowed` (`fixes/108`) —
  `source_skip()`/`source_peekn()` always treated a backslash-newline as
  a line continuation, even inside single quotes (and a heredoc with a
  quoted delimiter, which shares the same code path — POSIX requires
  both to be completely literal). A new `source_squoted` flag
  (`source.h`), set by `parse_squoted.c` around its own read loop, is
  how these primitives — which sit below the parser, with no access to
  its quoting state — know to skip continuation-removal.
- `cmdsubst-does-not-isolate-shell-state` (`fixes/109`, found while
  writing `fixes/107`'s own regression tests, all `"$(set -e; ...)"`
  style) — `expand_command.c` (command substitution, `"$(...)"`/
  backquotes) never called `sh_push()`, unlike `eval_subshell.c`'s
  `"(...)"`, even though POSIX defines command substitution as a
  subshell too (2.6.3). `set -e`/any other `set` option, `cd`, `umask`,
  etc. run inside `"$(...)"` permanently changed the *calling* shell's
  own state once the substitution finished. This is very plausibly what
  was actually behind `confdefs-h-duplication` above.

What's left is whatever the next triage pass over these suites turns up.
`tests/yash/random-y.tst` itself (formerly `BUGS: yash-random-y-tst-hangs`)
turned out to already be fixed as a side effect of unrelated 2026-07-30
work by the time it was next checked — running it for real (instead of
timing out) turned up three genuine `$RANDOM` bugs, fixed as `fixes/113`.
A related-looking `redir-p.tst` hang found on 2026-07-29 turned out to be
a real, deterministic bug once properly isolated (an 8-deep `<&` dup
chain off a freshly-opened fd resolved before its source ever got a
chance to open) — fixed as `fixes/89` (fd-resolution ordering, both the
builtin and forked-external-command paths) plus `fixes/88` (`builtin_cat()`
spinning instead of erroring on the resulting bad fd).

`DO_YASH_TESTS` stays off by default regardless — a full sweep of all 119
`tests/yash/*.tst` files (2026-07-30) found several *other* files
(`arith-y.tst`, `cmdprint-y.tst`, `pipeline-y.tst`, `redir-y.tst`,
`until-y.tst`, `while-y.tst`) that still hang, none yet isolated to a
specific case; see `BUGS: yash-suite-other-hangs`.

A 2026-08-08 pass specifically triaged `tests/posix` failures (not the
`tests/yash` suite above), starting from `arith-p.tst` (17/65 passing)
and finding seven independent, real arithmetic-expansion bugs chased
down together (`fixes/141`-`145`): a segfault on any *chained* unary
operator (`$((-+-2))`, `parse_arith_unary.c` recursed into
`parse_arith_value()`, primaries-only, instead of back into itself);
an unset/empty variable in arithmetic context producing no output
instead of the POSIX-mandated `0`; `&&`/`||` silently misparsed as
one-character `&`/`|` (the bitwise branch in `parse_arith_binary.c`
only excluded a following `=`, never a repeat of the same character);
`&&`/`||` never actually short-circuiting (`expand_arith_binary.c`
evaluated both operands unconditionally, so `0 && (a=5)` still ran the
assignment); the same "&"-repeat blind spot for `<`/`<<` and `>`/`>>`,
breaking chained shifts (`1<<2<<1`); the operator-precedence search
loop decrementing its level variable even after already matching on
the very first check, so a later, tighter operator got left for an
ancestor frame to wrongly re-group at a looser precedence
(`1+2*3` → `(1+2)*3`); and legacy backquoted command substitution
never being recognized as a valid arithmetic operand at all. Brought
`arith-p.tst` to 32/43 (remaining gaps — `?:` unimplemented, `&`/`^`/`|`
and `&&`/`||` each flattened into one shared precedence level instead
of the distinct ones C/POSIX specify, and arithmetic assignment not
rejecting a readonly target — filed individually in `BUGS`, not yet
fixed).

A later pass (`fixes/150`, `fixes/151`) closed the `?:` and
precedence-flattening gaps: added `parse_arith_ternary.c` as its own
right-associative, short-circuiting precedence level above the binary
operator chain, and split `parse_arith_binary.c`'s single combined
bitwise (`&`/`^`/`|`) and logical (`&&`/`||`) branches into their
correct distinct C/POSIX levels. Brought `arith-p.tst` to 42/43. The
readonly-assignment case was later fixed (`fixes/152`) by adding a
readonly check to `var_setv()` and `var_setvint()` and propagating
`V_READONLY` from parent to child variables in `var_create()`,
bringing `arith-p.tst` to 43/43.

The same pass then found a much higher-leverage bug while chasing an
unrelated `break`-argument test: `eval_for()`/`eval_loop()` (`for`/
`while`/`until`) run their loop body against the *caller's* eval frame
(needed so `break`/`continue`/`$?`/`errexit` state inside the body stay
visible to the rest of the script) but push their own, separate frame
purely so a jump has something to target — and then returned *that*
frame's own `exitcode` field, which nothing had ever written, instead
of copying over what the body actually left the shared frame at.
Confirmed via `gdb` watchpoint on `sh->exitcode`: it flipped from a
body command's real status back to a hardcoded `0` the instant the
loop's own frame got popped. This was invisible whenever a *later*
command in the exact same top-level list (`for ...; done; echo $?` all
on one line) happened to read `$?` first, which is why it went
unnoticed this long — but a loop sitting on its own lines, the
ordinary way real scripts are written, silently clobbered `$?` to `0`
right after every single `for`/`while`/`until`, regardless of how the
loop's body actually exited. Fixed (`fixes/146`, `147`) by tracking the
body's real last-run status (for `while`/`until`, snapshotted
immediately after each body run, since the *next* iteration's own test
re-run otherwise overwrites it first) and syncing it back to both the
loop's own pushed frame and the global `sh->exitcode` directly — the
same pattern `eval_subshell()` already used for `(...)`, just never
applied here.

Chasing a segfault this surfaced (`for i in ; do ... done` inside a
command substitution) found a second, independent bug in the same
function: `eval_for()` distinguished a real `for x in <words>` from a
bare `for x; do` (which POSIX says must fall back to iterating the
positional parameters) purely by whether the parsed argument list was
non-`NULL` — indistinguishable from an *explicit*, merely empty `in`
list (`for x in ; do`), which also parses to `NULL`. Fixed (`fixes/148`)
by adding a real `has_in` flag to `struct nfor`, set at parse time.

Finally, `break N`/`continue N` with `N` greater than the actual
enclosing-loop nesting depth (POSIX/bash: not an error, just targets
the outermost loop reachable) turned out to always silently do
nothing instead, even for a single ordinary loop at the very top of a
plain script. `eval_jump()`'s search discarded an already-matched loop
the instant it walked as far as a function/subshell/root boundary with
leftover, unsatisfied levels — conflating "asked for more levels than
exist here" with the real escape case the check exists to prevent (no
loop matched *at all* yet). A plain top-level script's own eval frame
carries the same `E_ROOT` flag a real subshell boundary does (via
`sh_loop()`'s own `E_ROOT | E_LIST` tempflags), which is what made this
bite even the most ordinary case. Fixed (`fixes/149`) by stopping the
search at the boundary without nulling an already-found match. One
related, deliberately unfixed case remains open (`break`/`continue`
run via `eval` inside a loop still no-ops, since `eval`'s own internal
frame reuses the same `E_ROOT` flag for an unrelated purpose) — see
`BUGS: break-continue-inside-eval-no-op`.

Net effect on the specific files touched: `arith-p.tst` 17/65 → 32/43,
`for-p.tst` → 19/20, `while-p.tst`/`until-p.tst` → 14/16 each,
`break-p.tst` → 31/32. The full `tests/posix` suite (144 files) still
has real failures in many other, unrelated areas — see `BUGS` for the
ternary/precedence/readonly arithmetic gaps and the `sigcont`/`sighup`/
`sigint`/`sigquit`/`sigterm`/`sigurg` trap-disposition family (24 files,
one shared but not yet root-caused cause) filed the same day.

---

## Goal 2 — `src/job` cleanup

Small, isolated, low-risk items left over from a full audit of
`lib/sig`/`lib/wait`/`src/job` (most of that audit's findings are already
fixed — see `fixes/41` through `fixes/76`):

- **Delete `src/job/job_x.c`.** Byte-for-byte duplicate of
  `job_printstatus.c`, confirmed zero callers anywhere.
- **Delete `job_get`/`job_proc`/`proc_bypid`** (`src/job.h`,
  `src/job/job_get.c`) — confirmed zero callers anywhere.

---

## Goal 3 — arena allocator for the AST (planned, not started)

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

## Goal 4 — `fd`/`fdtable`/`fdstack`/`redir`: the fd≤2 protection is load-bearing, not incidental

Grew out of chasing `BUGS: exec-redirection-and-error-broken` (`exec >&2
2>/dev/null; echo reached` sending "reached" to the wrong stream). Two
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

1. **The original `BUGS: exec-redirection-and-error-broken` symptom
   itself is still unfixed** (`exec >&2 2>/dev/null; echo reached`
   sends "reached" to the wrong stream — see `BUGS` for the full
   root-cause writeup, already done). The fix that worked for this
   specific symptom last time — forcing `redir_dup()` to eagerly
   resolve via `fdtable_dup(nredir->fd, FDTABLE_FORCE | FDTABLE_CLOSE)`
   right after `fd_dup()` sets up the pending dup, in
   `src/redir/redir_dup.c` — was reverted twice, both times because of
   bugs that are now fixed (the segfault, and the fd≤2-masked
   corruption). **It has not been retried since those fixes landed.**
   That's the single highest-leverage next step: reapply that one-line
   change (see `BUGS` for the exact diff/flags) and run the same
   stash-and-diff regression comparison described above. If it's
   clean, this closes the original bug this whole Goal grew out of.
2. **No general fdstack-scoped ownership tracking** (problem 1 below,
   unstruck). `fd_expected`/`fd_list[]`/`fd_top`/`fd_lo`/`fd_hi` are
   still bare process-global variables everywhere except the one call
   site (`eval_subshell()`) that now explicitly saves/restores them.
   Any *other* code path that pushes a temporary fdstack level and
   expects the real-fd bookkeeping to be clean on the way back out —
   there is currently no reason to believe there are none — could hit
   the identical class of bug in a different call site. No such case
   is currently known/reproduced; this is a standing architectural
   risk, not a confirmed live bug.
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
   No known live repro currently demonstrates a *wrong output* (as
   opposed to the now-fixed crash) from this gap — finding or ruling
   out one would be a good first step before attempting the refactor.

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
see "Current status". `redir_dup.c`'s eager-dup2() change is still
reverted/not retried — that remains the actual next step, also per
"Current status".)* `BUGS: exec-redirection-and-error-broken` still has
its own writeup of the narrower, original symptom and repro steps.

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

**Still open, and now unblocked**: `BUGS: exec-redirection-and-error-
broken`'s original, narrower symptom (`exec >&2 2>/dev/null; echo
reached` going to the wrong stream). The eager-`fdtable_dup()`-in-
`redir_dup.c` approach that fixed it last time (before hitting the
now-fixed segfault) can be retried on firmer ground — both bugs that
made it unsafe (the segfault, and the fd≤2-masked corruption this pass
root-caused) are fixed now. See `BUGS`'s own entry for the exact
approach and where it left off.

---

## Also open

- **Line-editing/terminal-abstraction/key-bindings rewrite** — a
  design-sized project inherited from the old `TODO` file, not a fixable
  bug. Minimal filename tab-completion (`src/term/term_complete.c`) is the
  only piece of this done so far.
