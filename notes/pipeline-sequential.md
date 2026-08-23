# Sequential pipeline fallback for fork()-less targets

`eval_pipeline()` (`src/eval/eval_pipeline.c`) always calls `job_fork()`
for every pipeline stage, on every platform, unconditionally. On a
platform with no real `fork()`, that's not a slow path -- it's a
silently broken one. This document is the plan to give `eval_pipeline()`
a second, sequential implementation for those platforms: instead of
`pipe()` + `fork()` wiring stdout of stage *N* to stdin of stage
*N+1* concurrently, each stage runs to completion in-process, in
order, writing its stdout into an in-memory buffer via `fd_subst()`
(the same mechanism `$(...)` already uses) and handing that buffer to
the next stage's stdin via `fd_here()` (the same mechanism here-docs
already use).

This is a sibling document to
[`cooperative-pipeline.md`](cooperative-pipeline.md), not a
replacement for it -- that one is about avoiding `fork()`'s *cost* on
platforms that have it, using real fibers to keep stages genuinely
interleaved. This one is about platforms that have no `fork()` at
all, where correctness, not performance, is the goal, and full
buffering between stages (no streaming, no interleaving) is an
acceptable trade because it's the only option anyway.

## The problem, confirmed

`doc/wasm.md` already documents the limitation in user-facing terms:

> What cannot work in the browser is anything that forks: pipelines
> between two external commands, background jobs, `$(...)` that runs
> a program. There are no processes to fork.

Reading the actual failure mode in `job_fork.c`/`eval_pipeline.c`
confirms it's worse than "cannot work" -- it fails *silently*:

- Emscripten's `fork()` (`system/lib/libc/musl/src/process/fork.c` in
  the emsdk tree) is a real, linkable symbol -- musl's libc ships it
  unconditionally -- but the underlying syscall has no implementation
  in a single-threaded wasm module and fails at runtime. It does not
  fail to link; it fails to run.
- `job_fork()` (`src/job/job_fork.c:52-56`) handles that failure:
  `if((pid = fork()) == -1) { ...; sh_error_errno("fork failed");
  return -1; }` -- so far so good, it does return `-1`.
- `eval_pipeline()` (`src/eval/eval_pipeline.c:112-124`) does not
  check for that: `pid = job_fork(job, node, npipe->bgnd); if(!pid) {
  ...exit(...); } else { ...debug_ulong("forked", pid, 0)... }`.
  `-1` is truthy in C, so `!pid` is false, and the *parent* branch
  runs -- treating a fork that never happened as if it had succeeded,
  with a fabricated child pid of `-1`. That `-1` gets stored as
  `job->procs[index].pid` and later handed to `job_wait()`, whose
  underlying `waitpid(-1, ...)` call means "wait for any child in my
  process group" per POSIX -- not "the child I meant," because there
  is no such child.
- Net effect: a pipeline silently produces none of its expected
  output and reports a plausible-looking `$?`, rather than erroring.
  This wasn't reproduced end-to-end in a running wasm build as part of
  writing this document (the local harness for driving the Emscripten
  glue via bare `node` wasn't worth fighting further for this) -- but
  the code path above is unambiguous by direct reading, and matches
  `doc/wasm.md`'s own description of the limitation.

This is a real, if silent, defect -- not just a missing feature. It
should probably get a `BUGS` entry of its own regardless of whether
this plan is picked up.

## Existing precedent: `expand_command()`

Shish already has a fork-free, fully in-process way to run a subtree
and capture its stdout: `expand_command()` (`src/expand/expand_command.c`),
which implements `$(...)`. Command substitution is specified as
running in a subshell (POSIX 2.6.3), and shish already honors that
*without forking*, by saving and restoring every piece of state a real
subshell's copy-on-write process would otherwise isolate for free:

```c
fdstack_push(&fdst);
fd_state_save(&fdstate);
fd_push(&fd, STDOUT_FILENO, FD_WRITE);
fd_subst(&fd, &sa);              /* stdout -> in-memory stralloc */

vartab_push(&vars, 0);           /* variable scope */
sh_push(&she);                   /* shell options, cwd, umask, ... */
exec_functions_save(&funcs);     /* function table */
traps_snap = trap_snapshot_save();

eval_push(&en, E_ROOT);
en.jump = 1;
jmpret = setjmp(en.jumpbuf);
if(jmpret) {
  en.exitcode = (jmpret >> 1);
} else {
  eval_tree(&en, cmd->list, E_LIST);
}
ret = eval_pop(&en);

trap_snapshot_restore(traps_snap);
exec_functions_restore(&funcs);
sh_pop(&she);
vartab_pop(&vars);

fdstack_pop(&fdst);
fd_state_restore(&fdstate);
```

This is directly reusable. POSIX pipeline semantics (2.9.2) specify
that *each* pipeline component runs in its own subshell environment
too -- shish's current `job_fork()`-based implementation already
matches that by forking every stage, including the last one (no
"lastpipe" optimization). So `expand_command()`'s save/restore block
isn't an approximation of pipeline-stage isolation, it's the same
isolation pipeline stages are already specified to have -- this plan
runs each stage through exactly that block instead of a fork, once
per stage.

`fd_here()` (`src/fd/fd_here.c`) is the read-side counterpart, already
used for here-documents: it wraps a `stralloc` as a readable buffer
(`buffer_fromsa()`), with its `deinit` set to free the `stralloc`'s
storage once the fd is closed. That's the exact mechanism needed to
feed one stage's captured output into the next stage's stdin.

## Detecting "no real fork" -- DONE

This turned out not to need a hand-maintained preprocessor macro after
all (this section originally proposed one, for the reason given
below) -- it's implemented as a `cmake/Checks.cmake` capability check,
`HAVE_FORK`, wired into `config.h` the same way every other `HAVE_*`
flag in this codebase already is (`#cmakedefine HAVE_FORK 1`).

The subtlety a plain `check_function_exists(fork HAVE_FORK)` can't
handle on its own: it would report `fork` as present on Emscripten,
confirmed directly (`emcmake cmake` prints "Looking for fork -
found"), because the symbol links fine (musl provides a real, callable
`fork()`) even though the underlying syscall has no implementation in
a single-threaded wasm module and fails at runtime (`ENOSYS`). The
opposite false result happens on native Windows: no libc-provided
`fork()` exists for the check to find, yet `lib/unix/fork.c` supplies
one (via `RtlCloneUserProcess`) that `job_fork()` already links
against there. Both cases are hardcoded overrides in `Checks.cmake`
rather than trusted to the check; Cygwin and MSYS are explicitly
carved back *out* of the Windows override (both provide a real POSIX
`fork()` from their own C library) so they go through the normal check
like any other POSIX target.

Implemented in `cmake/Checks.cmake` (search `HAVE_FORK` there for the
full logic and its comments) and `config.h.cmake`. One more wrinkle
surfaced while verifying this against every build tree already in
this repo's own `build/`: `CMAKE_SYSTEM_NAME` isn't a reliable signal
for either Emscripten or MSYS in practice here, because `cfg-cmake.sh`
never actually gets their real CMake toolchain file wired in (see the
new `BUGS` entries `cfg-emscripten-toolchain-file-never-resolves` and
the existing `cfg-cmake-mingw-silently-builds-native`) -- so the
`Checks.cmake` logic detects both by compiler name/path instead,
matching what `CMakeLists.txt`'s own `EMSCRIPTEN` variable already has
to do for the same reason. Verified against every `build/<triple>`
already configured in this repo: `x86_64-linux-gnu` and both `*-msys`
trees correctly run the real check and get `HAVE_FORK=1`;
`x86_64-w64-mingw32` gets the hardcoded `1` without running the check;
`build/emscripten` gets the hardcoded `0`.

**Still open, not yet implemented: a way to force the sequential path
on a normal dev machine**, so it can be tested without a wasm/WASI
runtime for every check -- e.g. a CMake option
(`-DFORCE_SEQUENTIAL_PIPELINE=ON`, default `OFF`) consumed in
`eval_pipeline.c` alongside `HAVE_FORK`:

```c
#if defined(FORCE_SEQUENTIAL_PIPELINE) || !defined(HAVE_FORK)
/* sequential path */
#else
/* existing job_fork() path */
#endif
```

Without this, the sequential path (once it exists) only ever runs on
a platform this repo isn't developed on day-to-day -- exactly the
situation `CLAUDE.md` already calls out for `WINDOWS_NATIVE`-only code
(verify by building for the target and confirming clean compile/link,
but that's a much weaker check than actually running `tests/*.sh`
against it). A force-flag turns this into a normal, CI-able,
native-Linux-testable code path instead.

## The core algorithm

New static helper in `src/eval/eval_pipeline.c`, `eval_pipeline_sequential()`,
called from the top of `eval_pipeline()` under the macro guard above;
the existing `job_fork()`-based body is untouched otherwise.

Walks the same list `eval_pipeline()` already walks
(`npipe->cmds`/`->next` today; `npipe->left`/`->right` if
[`pipeline-binary-refactor.md`](pipeline-binary-refactor.md) lands
first -- see "Interaction with the binary-operator refactor" below).
For each stage, in order:

```c
stralloc captured;         /* this stage's stdout, if it isn't last */
struct fdstack fdst;
struct fd_state fdstate;
struct fd out_fd, in_fd;
struct vartab vars;
struct env she;
struct func_snapshot funcs;
void* traps_snap;
struct eval en;
int jmpret, ret;
int is_last = (node->next == NULL);

stralloc_init(&captured);
fdstack_push(&fdst);
fd_state_save(&fdstate);

if(!is_last) {
  fd_push(&out_fd, STDOUT_FILENO, FD_WRITE);
  fd_subst(&out_fd, &captured);      /* stdout -> this stage's buffer */
}
/* last stage: stdout stays whatever the pipeline's own outer fd
   context already has wired -- the real destination, uncaptured */

if(prev_captured != NULL) {
  fd_push(&in_fd, STDIN_FILENO, FD_READ);
  fd_here(&in_fd, prev_captured);    /* stdin <- previous stage's buffer;
                                         fd_here() takes ownership, frees
                                         it when this fd is popped/closed */
}

vartab_push(&vars, 0);
sh_push(&she);
exec_functions_save(&funcs);
traps_snap = trap_snapshot_save();
eval_push(&en, E_ROOT);
en.jump = 1;
jmpret = setjmp(en.jumpbuf);
if(jmpret)
  en.exitcode = (jmpret >> 1);
else
  eval_tree(&en, node, 0);      /* NOT E_EXIT -- see callout below */
ret = eval_pop(&en);

trap_snapshot_restore(traps_snap);
exec_functions_restore(&funcs);
sh_pop(&she);
vartab_pop(&vars);

fdstack_pop(&fdst);
fd_state_restore(&fdstate);

sh->exitcode = ret;   /* only the last stage's ret ends up mattering --
                          same as today, where $? is the pipeline's
                          last member's status */

prev_captured = is_last ? NULL : &captured;   /* handed to fd_here()
                                                  next iteration; do
                                                  NOT stralloc_free()
                                                  it here */
```

Points worth calling out explicitly:

- **Must not pass `E_EXIT`.** The sketch above deliberately calls
  `eval_tree(&en, node, 0)`, not `job_fork()`'s child's
  `eval_tree(e, node, E_EXIT)`. `E_EXIT` doesn't mean "the process is
  exiting" in the everyday sense -- per `eval_cmdlist.c`'s own comment,
  it means "exec the tail command directly instead of forking": it
  tells `eval_simple_command.c` (`X_EXEC`, line ~303) that it's safe
  to `execve()` *this very process* for an external command's tail
  position, because the process is disposable -- a forked child about
  to `exit()` regardless. In the sequential fallback there is no
  disposable child; this *is* the one real, ongoing shell process.
  Passing `E_EXIT` here would make a trailing external command
  `execve()` over the whole shell mid-script. `expand_command()`
  already gets this right (no `E_EXIT` in its own `eval_tree()` call)
  for the identical reason -- it doesn't fork either.
- **Every stage is isolated**, including the last one -- matching
  `job_fork()`'s current behavior of forking every stage, not
  bash/ksh's optional "lastpipe" behavior. A variable assignment,
  `cd`, or trap set inside any pipeline stage (including the last)
  should not leak into the calling shell's environment, same as today.
- **Only non-last stages get their stdout captured.** The last
  stage's stdout is left wired to whatever the pipeline's own outer
  fd context already provides -- it's the only stage whose output
  actually needs to reach the real world.
- **A stage's own redirections still apply normally.** `fd_push()` +
  `fd_subst()`/`fd_here()` here only sets up the *default* stdin/stdout
  before `eval_tree()` runs, exactly like `job_fork()`'s child does
  today (see the comment in `job_fork.c` about a real explicit
  redirection correctly overriding a pipe-supplied default) --
  `cmd1 2>err.log | cmd2` keeps working unchanged, since `cmd1`'s own
  `2>err.log` redirection is resolved by `redir_eval()` inside
  `eval_tree()`, layered on top of the default stdin/stdout this sets
  up first, same order as today.
- **Nested `$(...)` or `(...)` inside a stage already works.** Since
  `expand_command()` doesn't fork either, a stage that itself contains
  a command substitution or subshell already goes through this same
  save/restore machinery today, with no interaction to design for.
- **No streaming.** Stage *N* runs to completion -- including any
  children, for a compound-command stage like `{ a; b; } | c` -- before
  stage *N+1* starts at all. This is strictly less capable than real
  concurrent pipes (a stage that reads and writes interleaved with its
  neighbor, or that never terminates, won't work here the way it does
  under `job_fork()`), but it's what "no real process" ultimately
  allows; it's the same limitation real command substitution already
  has for its own subtree.

## What this does *not* fix

- **External-command stages.** A pipeline stage that execs an
  external program still can't run at all on these platforms -- there
  is no process to exec into, independent of pipes or forking. This
  plan only helps a pipeline whose stages are all shish builtins
  (compound commands built from builtins are fine too, e.g.
  `{ echo a; echo b; } | cat`). This is the same scope boundary
  `cooperative-pipeline.md`'s "Next steps" step 1 already names for
  its own (different) reason -- worth keeping the two plans' scoping
  consistent if both get implemented.
- **Backgrounding (`cmd1 | cmd2 &`).** There's no way to "return
  immediately" when everything runs synchronously in one process --
  the whole premise of `&` requires something to keep running after
  the shell moves on, which needs a process or a thread, neither of
  which exists here. Resolved: `eval_pipeline_sequential()` errors
  loudly (`sh_error("background pipelines are not supported without a
  working fork()")`) rather than silently running synchronously --
  loud over silent, matching the entire reason this path exists.

  **Caveat found during implementation:** for the common top-level
  `cmd1 | cmd2 &` case, this guard never actually runs.
  `eval_node_bgnd()` (`src/eval/eval_node_bgnd.c`) is what top-level
  sibling-chain evaluation calls instead of a bare `eval_node()`, and
  it does its own unconditional `job_fork()` *before* `eval_pipeline()`
  is ever reached; on a real forkless target that `job_fork()` call
  fails, and `eval_node_bgnd()` has the exact same `if(!pid)`
  fork-failure mishandling this document's "The problem, confirmed"
  section already described for `eval_pipeline()` -- except here it's
  worse: the branch that would run `node` (and eventually reach
  `eval_pipeline_sequential()`'s guard above) is skipped entirely,
  and the function falls through to `return 0`, silently doing
  nothing instead of erroring. Filed as
  `eval-node-bgnd-silent-on-fork-failure` in `BUGS`; fixing it is a
  separate change from this document's scope (it's not a pipeline
  problem specifically -- `eval_node_bgnd()` covers any backgrounded
  compound command, not just pipelines).

## Interaction with the binary-operator refactor

[`pipeline-binary-refactor.md`](pipeline-binary-refactor.md) proposes
reshaping `struct npipe` from an n-ary `ncmd`/`cmds` list to a binary
`left`/`right` node, with `eval_pipeline()` flattening the left spine
into a local array before its fork loop runs. If that lands first,
`eval_pipeline_sequential()` should walk that same flattened array
rather than duplicating traversal logic -- the sequential algorithm
above doesn't care how the stage list was produced, only that it has
one, in left-to-right order, with a way to tell the last stage apart.
If this document is implemented first instead, its own traversal
(`npipe->cmds`/`->next`, matching today's shape) is a straightforward,
mechanical swap to the flattened-array form once that refactor lands.

## Files to touch

1. ~~`cmake/Checks.cmake` / `config.h.cmake` -- add the `HAVE_FORK`
   check described above.~~ **Done.**
2. ~~`configure.ac`~~ **Done** -- added the matching `HAVE_FORK` check
   for the autotools path (it had none at all before), via
   `AC_EGREP_CPP` against the exact same predefined-macro conditions
   `lib/windoze.h`'s `WINDOWS_NATIVE` already uses (rather than
   host-triple matching, since `AC_CANONICAL_HOST` is disabled here --
   see the `dnl` in `configure.ac`'s "check host and target" section --
   and re-enabling it for this alone seemed like a bigger, riskier
   change than asked for). Verified the preprocessor logic directly
   against native gcc, `x86_64-w64-mingw32-gcc`, and `emcc`; verified
   `autoreconf`/`autoconf` compile it into the generated `configure`
   correctly. Full end-to-end `./configure` → `config.h` generation
   could *not* be verified in this repo's autotools setup specifically
   -- it has a pre-existing, unrelated gap where `AC_OUTPUT` is never
   invoked in `configure.ac`, so `config.h` never actually gets
   written regardless of this change; confirmed that gap predates this
   edit, out of scope to fix here.

   **Still open:** `CMakeLists.txt`/`cmake/Checks.cmake` (and now
   `configure.ac` too) still need the `FORCE_SEQUENTIAL_PIPELINE`
   test-only override option described above -- neither build system
   has it yet.
3. ~~`src/eval/eval_pipeline.c` -- add `eval_pipeline_sequential()`,
   branch to it from `eval_pipeline()`'s top under `#if
   !defined(HAVE_FORK)`~~ **Done** (`fixes/212-eval-pipeline-sequential-no-fork.patch`).
   Verified via a throwaway native build with `HAVE_FORK` forced
   `FALSE` (see `tests/fixed.sh`'s fixes/212 entry for the exact
   method and what was checked): builtin-only and multi-stage
   pipelines match the normal fork() path's output, exit status
   propagates, non-last stages stay isolated, and a backgrounded
   pipeline errors loudly. The `eval_node_bgnd()` caveat above was
   found during this verification.
4. ~~`BUGS` -- add an entry for the silent-failure bug described above
   (mishandled `fork() == -1` in `eval_pipeline()`)~~ **Done**
   (`eval-pipeline-silent-on-fork-failure`), independent of whether
   this plan is implemented -- it's a real defect on its own. Also
   added `cfg-emscripten-toolchain-file-never-resolves`, found while
   verifying step 1 against this repo's own `build/emscripten`.

## Testing

Per `CLAUDE.md`'s testing conventions: add `tests/pipeline-sequential.sh`
(or fold into whatever `tests/pipeline.sh` `cooperative-pipeline.md`
already recommended adding), built with `-DFORCE_SEQUENTIAL_PIPELINE=ON`
so it runs on every normal CI target, not just an emscripten/WASI one.
Cover:

- A builtin-only pipeline's output and `$?` match the same pipeline
  run with the flag off (the two code paths should be
  behaviorally identical for anything that doesn't depend on real
  concurrency).
- A variable/`cd`/trap change inside a non-last stage doesn't leak to
  the calling shell (the isolation this plan depends on).
- A stage with its own explicit redirection alongside the pipe
  (`cmd1 2>err.log | cmd2`) still redirects correctly.
- A pipeline containing an external-command stage fails clearly
  (whatever "clearly" is decided to mean) rather than silently, per
  "What this does not fix" above.
- Once implemented for real: an actual `cfg-emscripten`/`cfg-wasm`
  build, confirming `echo hi | cat` (today's silent-failure repro)
  produces the right output there too, not just under the force-flag
  on native.
