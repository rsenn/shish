# TODO / Roadmap

Leverage-sorted list of what's still open. Fixed work lives in `git log` and
`fixes/*.patch`, not here — this file only tracks what's left to do. See
`BUGS` for confirmed, reproducible defects with repro steps.

---

## Goal 1 — POSIX conformance

`./configure` (this project's own stress test — autoconf output exercises
nested command substitution, heavy fd juggling, `eval`, here-docs,
trap/exit interplay) now runs to completion end-to-end. The `tests/posix`
conformance suite (120 files) is wired into `ctest` and runs by default.
`tests/yash` (119 more files) is wired in the same way but gated behind
its own `-DDO_YASH_TESTS=ON` (off by default — `tests/yash/random-y.tst`
hangs and only terminates via its own 120s `TIMEOUT`, `BUGS:
yash-random-y-tst-hangs`, which otherwise dominates a default `ctest`
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

What's left is whatever the next triage pass over these suites turns up,
plus `BUGS: yash-random-y-tst-hangs`, found and narrowed (not yet fixed)
while doing exactly that on 2026-07-29 — now points at the file's own
busy-`until` `$RANDOM`-comparison loops rather than the later
subshell-piping cases originally suspected. A related-looking
`redir-p.tst` hang found the same day turned out to be a real,
deterministic bug once properly isolated (an 8-deep `<&` dup chain off
a freshly-opened fd resolved before its source ever got a chance to
open) — fixed as `fixes/89` (fd-resolution ordering, both the builtin
and forked-external-command paths) plus `fixes/88` (`builtin_cat()`
spinning instead of erroring on the resulting bad fd). What first
looked like Release-vs-MinSizeRel build sensitivity for that one turned
out to be two stacked testing mistakes instead (see `BUGS`) — a caution
for `yash-random-y-tst-hangs`'s own still-unconfirmed build-sensitivity
claim, not evidence either way.

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

## Also open

- **Line-editing/terminal-abstraction/key-bindings rewrite** — a
  design-sized project inherited from the old `TODO` file, not a fixable
  bug. Minimal filename tab-completion (`src/term/term_complete.c`) is the
  only piece of this done so far.
