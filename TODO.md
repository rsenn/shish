# TODO / Roadmap

Superseded the old `TODO` file (its "buggy"/"not implemented" lists are stale —
functions, tilde expansion, here-docs, job control, etc. are all in now).
This roadmap was produced by:

1. building fresh (`cmake -S . -B /tmp/build && cmake --build .`),
2. running `ctest`,
3. actually invoking this repo's own `./configure` under the freshly built
   `shish` binary and reading `config.log`,
4. cross-checking `fixes/*.patch` (18 already-committed, un-messaged fixes —
   `git log` only shows `...` as the message for most recent commits, so
   `fixes/` is currently the only readable changelog) against current
   `src/`.

Two goals, in order. Points inside each goal are sorted by leverage (fixes
that unblock the most other things, or that turn guesswork into a concrete
failure list, come first).

---

## Goal 1 — `shish` can run its own `./configure`

This is the project's own stress test for POSIX conformance (autoconf output
is a brutal, real-world corpus: nested command substitution, heavy `>&5`/`>&6`
fd juggling, `eval`, here-docs, trap/exit interplay). It currently does not
get past `AC_PROG_CC`. Eighteen fixes already landed to get this far
(LINENO tracking, exit propagation through subshells/functions, pipeline exit
status, `$()`/function redefinition cascade-free bug, empty-field splitting,
etc. — see `fixes/*.patch` for the reasoning behind each). What's left:

1. **Fix the fd-duplication/re-execution bug that corrupts `configure`'s
   control flow.** Repro: copy the tree, `rm -rf build`, run
   `<shish-build>/shish ./configure`. Output duplicates entire "checking
   for gcc" / "checking whether the C compiler works" blocks over and over,
   interleaved, and a child process SEGVs mid-run (`job_printstatus`'s
   `"process N signaled: SEGV"`). `config.log` shows `$? = 0` immediately
   followed by `result: no` — i.e. the script's own bookkeeping ($?, the
   fd-5/fd-6 log-vs-screen split autoconf relies on) is getting corrupted,
   not just the terminal output. This is very likely the same class of bug
   BUGS already flags ("the fdtable resolving code can end up in an
   infinite recursion") interacting with autoconf's dense `{ ... } >&5`
   redirection nesting. **Highest leverage item in this list** — everything
   past `AC_PROG_CC` is invisible until this is fixed, and it likely also
   explains the eventual "C compiler cannot create executables" false
   negative (gcc genuinely works; rerun that check in isolation once this
   lands to confirm it clears on its own).
   - A minimal 3-line `for`/`>&5`/`>&6` fd-dup repro did *not* reproduce it,
     so the trigger needs more of autoconf's actual shape (nested `eval`,
     `case` inside the retry loop, or a second layer of subshell). Worth
     bisecting configure itself (binary-search which `AC_*` check block
     first shows duplicated output) rather than guessing.
   - **Minimal duplication repro found and fixed (2026-07-21), no cmdsubst
     needed:** `(true | true)` — a pipeline inside a subshell — made
     everything after the subshell run once per surviving builtin
     pipeline member. Root cause: `eval_pipeline()`'s forked children
     call `sh_exit()` (via an `E_EXIT`-flagged `eval_tree()` call) to
     terminate, but `sh_exit()`/`eval_exit()` `longjmp()` to the
     nearest jump-enabled `struct eval` frame instead of exiting
     whenever the (process-global) `eval` chain has one — and
     `eval_subshell()` always pushes exactly such a frame via
     `setjmp()`, which is still a physically valid stack address in a
     forked child's copy of the stack. The child's "exit" landed back
     inside the parent-context's `eval_subshell()` call (now running
     in the child), which returned normally into the top-level
     `eval_tree()` loop — so the "exited" child kept interpreting the
     rest of the script as an extra copy of the shell. Fixed in
     `sh_forked()` (runs on every fork path) by clearing `jump` on
     every frame in the inherited `eval` chain, so `eval_exit()` never
     finds a stale target in a forked child and always falls through
     to a real `exit()`. This is likely the exact bug behind
     configure's duplicated-output symptom (autoconf's checks are full
     of subshells wrapping pipelines).
   - **Fd-side root cause found and fixed (2026-07-21):** the resolver
     endlessly `dup()`ed until EMFILE, then spun at 100% CPU: a stale
     `fd_expected` made the `d->n == fd_expected` branch in
     `src/fdtable/fdtable_dup.c` retry `dup()` under `FDTABLE_FORCE`
     forever (leaking one fd per iteration), and `fdtable_lazy()` looped
     on `FDTABLE_ERROR`. Fixed by letting the `dup()` bet be tried at
     most once (the forced retry now goes straight to `dup2()`) and by
     propagating `FDTABLE_ERROR` out of `fdtable_lazy()`. Repro that
     used to storm (~3700 dups, now ~750 with no runaway chains):
     `sed -n 1,2053p configure > /tmp/storm.sh && strace -f -e trace=dup
     ./shish /tmp/storm.sh`. A Debug-build `./configure` now runs in
     seconds up to the `AC_PROG_CC` false negative.
   - **Command substitution of a pipeline always expanding to empty,
     fixed (2026-07-21):** root-caused with gdb -- `fd_subst()` only
     sets up an in-process memory sink, but a pipeline always forks
     real children (even for builtins), and nothing wired a real pipe
     from the forked last member back into that sink.
     `eval_pipeline.c` now reuses the same `fdstack_npipes()`/
     `fdstack_pipe()`/`fdstack_data()` machinery `exec_program.c`
     already relied on for the (pipeline-free) substitution case,
     restricted to the pipeline's last member and to the closest
     fdstack level (`fdstack_npipes()`/`fdstack_pipe()` used to walk
     the whole chain unbounded, which is what broke a substitution
     nested inside another one). See `fixes/31`. Verifying this
     surfaced a separate, unrelated, pre-existing bug via an ASan
     build: `job-fork-nproc-oob` (`BUGS`) -- any 2+-stage pipeline,
     substituted or not, corrupts its own job's proc array.
   - Still open, in likely-blocking order: `job-fork-nproc-oob` (just
     found, above), the `AC_PROG_CC` "cannot create executables" false
     negative, and the MinSizeRel-only early death (`BUGS` has the
     details).
   - **`fd_expected` staleness and the leaked kernel fds behind it fixed
     (2026-07-21):** the leaks themselves (pipe-stdin fd never closed
     after `dup2` onto 0, here-doc temp fds staying open) fell to the
     retry rework above; the remaining staleness had three sources, all
     raw `close()`s the table never learned about — `fd_mmap()` closing
     the script fd after mmap, `fdtable_resolve()`/`fdtable_dup()`/
     `fdtable_here()`/`fdtable_open()` closing old effective fds during
     resolution, and (the one that hit every script) `fd_close()`
     closing buffer-owned kernel fds like the command-substitution pipe
     read end, whose `fd->e` is -1 so the existing bookkeeping missed
     it. New `fdtable_untrack()` (inverse of `fdtable_track()`) is now
     called at every such site. Verified with temporary sh_loop
     instrumentation (compare `fd_expected` against `dup(0)` and scan
     `/proc/self/fd` vs `fd_list[]` after every toplevel command):
     silent across all of `tests/*.sh`, and `strace` shows zero stray
     `dup()`s where every redirection-heavy command used to pay one
     stray dup+close per miss.
   - Build a debug config (`cfg-cmake.sh`'s Debug type, or add
     `-DUSE_EFENCE=ON`/ASan) to get a real backtrace for the SEGV instead of
     just the job-control "signaled" line.

2. **Stop `sh_errorn` (`src/sh/sh_errorn.c`) from appending stray
   `strerror(errno)` to unrelated error messages.** It unconditionally does
   `if(errno) buffer_puts(fd_err->w, strerror(errno))` — but `errno` is
   whatever the last syscall anywhere left it, not necessarily related to
   the message being printed. Confirmed reproducible: `${Y?"Y is unset"}`
   with `Y` unset should print exactly `Y is unset` to stderr; it currently
   prints `Y is unset: Inappropriate ioctl for device` (a stale ENOTTY from
   an unrelated earlier `ioctl`, e.g. terminal probing on non-tty stdin) —
   this is what's now failing `tests/param.sh` on a fresh build. Every
   caller of `sh_error()`/`sh_errorn()` for a *custom* message (all the
   `${var?word}` cases, plus assorted builtin error paths) is at risk of
   this. autoconf's own `as_fn_error` helper and most `${VAR?msg}`-style
   probes pattern-match stderr text, so this corrupts exactly the kind of
   message autoconf depends on. Fix: only append the errno text at call
   sites that actually just failed a syscall (e.g. add a distinct
   `sh_error_errno()`/flag, and clear `errno = 0` before the syscalls that
   currently leak into unrelated later error calls), not unconditionally
   in the shared helper.

3. **Re-run `./configure` after 1 and 2 land and treat the next failure as
   the next task.** This is a self-refilling queue — don't try to
   pre-guess further than the current blocker.

---

## Goal 2 — Broader POSIX compliance

4. ~~Wire `tests/posix/*.tst` and `tests/yash/*.tst` into the build.~~
   **Done.** `tests/run-tst.sh` drives the upstream `run-test.sh` harness
   under `bash -O expand_aliases` (it needs `$LINENO` and alias expansion
   in non-interactive scripts, neither of which `dash`/plain POSIX `sh`
   provide) and turns its per-case result file into a single pass/fail,
   one CTest case per `.tst` file, named `posix/<file>.tst` /
   `yash/<file>.tst`. Runs by default as part of `ctest`/`make test`
   (gate it off with `-DDO_CONFORMANCE_TESTS=OFF` if you just want the
   fast core suite). As expected, this immediately surfaces a large,
   concrete failure list instead of the previous zero conformance
   coverage — e.g. a quick sample already found real gaps: `getopts`
   (most of it), functions as loop/if/case bodies, `&&`/`||` exit status
   in a couple of shapes, comments after `&`/in compound-command bodies.
5. Now that 4 is wired up, prioritize by what it reports — don't
   hand-pick POSIX sections to fix ahead of that data. (Not done in this
   pass — wiring it up was the deliverable; triaging the failure list is
   its own follow-on task.)
6. **Process note, not code:** most recent commits (`git log --oneline`)
   have the placeholder message `...`. `fixes/*.patch` is currently the
   only place the *why* for the last 18 fixes is written down, and it's an
   untracked directory (`git status` shows it as untracked) — it will not
   survive a clean checkout. Either fold that reasoning into real commit
   messages/`ChangeLog`, or commit `fixes/` deliberately; right now a
   `git clean` would silently delete the only documentation of why these
   fixes exist.

---

## Goal 3 — `lib/sig` / `lib/wait` / `src/job`: fix job control, cut dead code

Produced by reading every file in the three modules, grepping every call
site in `src/`, and verifying the standout claims live (debug+ASan build,
exercised `fg`/`bg`/background jobs through a pty). Sorted by leverage.

1. ~~Fix the `fg` out-of-bounds crash.~~ **Done (2026-07-22).** Rewrote
   `builtin_fg` to resolve a single job directly instead of building a
   `struct job *joblist[argc - 1]` VLA at all (fg only ever moves *one*
   job to the foreground — there's only one terminal to hand over), which
   sidesteps the zero-length-VLA write entirely rather than just special-
   casing it. See `fixes/41`.

2. ~~`job_fork()` blocks `SIGCHLD` before `fork()` and never unblocks
   it.~~ **Stale — already fixed by the time this was re-checked
   (2026-07-22):** `src/job/job_fork.c`'s parent branch now does
   `sig_unblock(SIGCHLD)` unconditionally before returning, and the child
   branch unblocks it too, right before the comment noting exactly why
   (a program it execs must not inherit the blocked mask).

3. ~~`fg`/`bg` don't implement job control's stop/resume half at
   all.~~ **Done (2026-07-22).** `builtin_fg`/`builtin_bg`
   (`src/builtin/builtin_jobs.c`) now call `job_foreground()`, send
   `SIGCONT` via `killpg()`, and block on `job_wait()` (fg) or leave the
   job running and print its `[n]+ command &` line (bg) — see
   `fixes/41`-`fixes/44`. This needed three supporting fixes beyond the
   builtin file itself, found while making fg/bg actually work end to
   end rather than just compile:
   - `job_wait()` never asked `wait()`/`waitpid()` for `WUNTRACED`, so a
     stopped process was never distinguishable from "still running" --
     it would just block until the process eventually exited. Added
     `wait_pid_untraced()`/`wait_nohang_untraced()` (`lib/wait/`, new)
     and wired them into `job_wait()`'s synchronous wait and
     `sh_onsig()`'s async SIGCHLD path, gated on `sh->opts.monitor` so
     script (non-interactive) behavior is unchanged.
   - `job_done(j)` (`src/job.h`) was `#define`d as `!job_running(j)`
     alone, which doesn't exclude a *stopped* job (`job_running()` only
     checks for `status == -1`, unset) -- a job that just stopped would
     look "done" and get `job_free()`'d (by `job_wait()` itself, and by
     `job_clean()`) instead of staying around for `fg`/`bg` to resume.
   - The real headline bug, found while chasing a spurious ~5s hang:
     `exec_program.c`'s second, independent fork path for `X_NOWAIT`
     background jobs (see item 6 below) never called `setpgid()` at
     all -- so a backgrounded *external* command (the common case,
     "cmd &") never actually got its own process group; `job->pgrp`
     recorded the child's pid as if it were one, but nothing made that
     true at the OS level. `killpg(job->pgrp, SIGCONT)` and
     `job_wait()`'s own `wait_pid(-job->pgrp, ...)` both therefore
     targeted a process group that didn't exist (`ESRCH`), silently
     doing nothing -- confirmed via `ps -o pid,pgid` showing the
     "backgrounded" child still sitting in the *shell's* process group.
     Fixed by adding the same double-`setpgid()`-in-both-parent-and-
     child dance `job_fork()` already does for its own path.
   - ~~A deeper, separate architectural gap found while working this:
     a plain *foreground* command never got a `struct job` at all
     (`exec_program.c`'s non-`X_NOWAIT` path was just
     `job_wait(NULL, pid, &status)`), so Ctrl-Z on one couldn't be
     resumed.~~ **Done (2026-07-22).** Gave this path a real
     `struct job` too (`job->bgnd = 0` so no redundant "Done" banner;
     `job->command` built from `argv` directly, since nothing
     downstream gets a chance to set it the way `eval_simple_command.c`
     does for a backgrounded job); also mirrored `job_fork()`'s
     `job_pgrp` bookkeeping, which the earlier fix's `tcsetpgrp()` call
     needed but didn't have -- without it, the terminal stayed with a
     since-exited child's defunct process group forever once a
     foreground command finished, wedging the shell's own next
     terminal read behind a `SIGTTIN`. See `fixes/48`.
   - Still open, not attempted: no `kill` builtin (signals are only
     reachable from fg/bg's own internal `SIGCONT`, not from scripts
     generally).

4. ~~`WAIT_EXITSTATUS` (`lib/wait.h`) and the fallback `WEXITSTATUS`
   (`src/job.h`) are both wrong`.~~ **Stale — already fixed** (both are
   `((status) >> 8) & 0xff` as of this re-check, 2026-07-22).

5. **Delete `src/job/job_x.c`.** It's a complete, byte-for-byte duplicate
   of `job_printstatus.c` under the name `job_status` — not declared in
   `job.h`, not called from anywhere. Zero-risk removal.

6. **`exec_program.c` has a second, independent fork path** (for
   `X_NOWAIT` background jobs) that skips the `sig_block(SIGCHLD)`-before-
   `fork()` guard `job_fork()` takes care to have (`// sig_block(SIGCHLD);`
   is commented out) — so it has the classic race where a fast-exiting
   child's `SIGCHLD` can be handled, and silently dropped by `job_bypid`
   finding nothing, before `job_new()` has registered the job. Two
   parallel, inconsistently-guarded fork/job-bookkeeping implementations
   existing side by side is a maintainability problem on its own; either
   unify them or at minimum add the missing guard to this path.

7. **`wait_nohang()` (`lib/wait/wait_nohang.c`) has no return statement
   on its `WINDOWS_NATIVE` branch** — falls off the end of a non-void
   function (undefined behavior) on native Windows builds. This is the
   function driving the SIGCHLD handler in `sh_main.c`, so child-process
   reaping is broken there. Cheap fix, even a `return -1` stub beats UB.

8. **Prune dead code**, mostly in `lib/sig`: of the ~20 declared
   functions/macros only `sig_block`, `sig_unblock`, `sig_blocknone`,
   `sig_catch`, `sig_name`, `sig_byname` are ever called from `src/`. The
   rest (`sig_push`/`sig_pusha`/`sig_pop`, `sig_restoreto`, `sig_shield`/
   `sig_unshield`, `sig_ignore`, `sig_pause`, `sigfpe`, `sigsegv`,
   `sig_number`, `sig_blockset`, `SIG_ALL`, the `sig_addset`/`sig_delset`/
   `sig_fillset`/`sig_emptyset` macros) only call each other in a closed
   loop nobody enters. `sig_pop` isn't even declared in `sig.h` — a total
   orphan. In `src/job`: `job_get`, `job_proc`/`proc_bypid` (inline in
   `job.h`) are never called from anywhere either. Either wire the
   signal-shielding functions up (they look like they were meant for the
   async-signal-safety issue below) or delete them — carrying ~15
   functions nothing calls is pure maintenance cost. While in there:
   `sig.h` defines `SA_MASKALL`/`SA_NOCLDSTOP` twice, back to back
   (copy-paste leftover), and `sig_push.c` reinterprets `sigset_t`
   (a 128-byte struct on glibc) through an `unsigned long*` — undefined
   behavior, currently harmless only because the function is unused.

9. **Lower priority, bigger scope:** `sh_onsig` (`sh_main.c`'s SIGCHLD
   handler) calls `term_erase()`/`term_restore()`/buffered I/O directly
   from signal-handler context — none of that is async-signal-safe. Not
   inside these three modules, but `job_signal()` is called from there, so
   it's adjacent. Worth its own pass: defer the terminal-repaint work to
   `job_update()` in the main loop instead of doing it inline in the
   handler.

10. Minor: `job_free()`'s `job_delete()` sets `job_pointer = NULL` when
    the current job is deleted rather than promoting the previous job to
    current (bash promotes "-" to "+"), so the "+"-marked job just
    disappears once it finishes. UX gap, not a crash.

---

## Old `TODO` file, investigated

Every item in the plain-text `TODO` file was individually checked against
current code and, where practical, against a live build. Real, confirmed
defects were moved into `BUGS` with precise repros; here's the evidence for
each conclusion. `TODO` itself has been trimmed down to just the handful of
items that are still genuinely open or unverified.

**Resolved / no longer true, removed from `TODO`:**

- *"leaving interactive mode e.g. in a forked subshell should free the
  prompt tree"* — not reproducible under the current architecture.
  `prompt_node`/`prompt_hash` (`src/prompt.h`) are plain process-wide
  globals, not part of `struct env`, so `sh_push`/`sh_pop` never touch
  them. `(...)` subshells (`eval_subshell.c`) don't fork at all — they're
  simulated in-process via `setjmp`/`longjmp` plus `vartab_push`/
  `sh_push`/`fdstack_push` — so they never re-parse a prompt either. Real
  forked children (anything going through `job_fork()`) never re-enter
  `sh_loop()`/`prompt_parse()`, so there's nothing for them to free before
  their `_exit()` reclaims everything anyway. Likely stale from a pre-2010
  version of the codebase (see below).

- *"fix iofd_exec() ???"* — `iofd_exec` never existed in this project's
  recorded git history at all (`git log --all -S"iofd_exec"` only matches
  the *first* commit, "imported from shish-0.7-pre3 tarball", and that
  commit's tree doesn't contain any file with "iofd" in the name either).
  The note predates git history. Its most plausible modern descendant,
  `fd_exec()`, was already found to be dead code (zero callers, superseded
  by `fdtable_here()`) and deleted in the `src/fd*` bug-fixing pass, and
  `fdtable_here()` itself was already reviewed and fixed there too.

- *"exporting of unset variables"* — works correctly today: confirmed live
  that `export FOO; FOO=bar` followed by running a child shows `FOO=bar`
  in its environment. `var_setsa()`'s `var->flags |= flags` (not `=`)
  preserves the `V_EXPORT` bit set by a prior bare `export FOO` across the
  later assignment. (Investigating this surfaced a real, separate crash —
  see `BUGS`: listing such a variable with `export -p` segfaults.)

- *"push variable stack only when needed?"* — already implemented where
  it matters: `eval_simple_command.c` only calls `vartab_push()` when
  `!(e->flags & E_EXIT) && cmd.ptr && cmd.id != H_SBUILTIN`, i.e. exactly
  skips it when unnecessary. The other 3 call sites (`eval_subshell.c`,
  `expand_command.c`, `exec_command.c`'s function-call path) push
  unconditionally, but each represents a genuinely new scope that needs
  its own variable table, not avoidable overhead.

- *"implement background processes and more sophisticated job control"*
  — superseded by Goal 3 above (which found `fg`/`bg` are actively broken,
  not just unsophisticated) plus a new finding: backgrounding a
  *non-simple* command has its own, distinct, more severe bug — see
  `BUGS`.

**Real, but different from how `TODO` described them — moved to `BUGS`
with the corrected/precise version:**

- *"redirections ... fdtable resolving code can end up in an infinite
  recursion"* — real; already mitigated with a depth cap in the `src/fd*`
  fixes, but the root cause (why a cycle would occur at all) is still
  open. Kept in `BUGS` in its more precise form.

- *"closing of file descriptors on >&-"* — real, and broader than first
  assessed. **Fixed** (`fixes/55-fd-close-noop.patch`): `fd_null()`
  (`redir_dup()`'s "-" branch) now clears `FD_READ`/`FD_WRITE` on the
  closed fd's entry instead of swapping in a null-sink buffer pair, so
  a later `fd_dup()` against it -- what both builtins' and external
  commands' own redirections go through -- fails exactly like a real
  closed descriptor, and `fdtable_resolve()` real `close()`s the
  underlying kernel descriptor right before an `execve()` would
  otherwise hand it to a child. Covers both the builtin case
  (`echo hi >&3` after `exec 3>&-` now correctly fails) and the
  external-command stdin cases found while leak-hunting (`cat <&-`,
  `cat < infile <&-`). Fixing it surfaced a separate, deeper bug in
  `fdstack_search()`/`fdtable_newfd()`'s scope-chain linking, also now
  **fixed** (`fixes/58-fdstack-scope-chain-mislink.patch`):
  `fdstack_search()` was walking past an ancestor scope's existing
  entry for a fd number instead of stopping there, so a nested scope's
  new entry for that same number got linked *underneath* the
  ancestor's instead of replacing it as the visible top -- simplified
  to a direct top-of-chain check, since under correct push/pop
  discipline there's never anything useful further down the chain to
  walk into.

- *"conform to 3.9.1.1 Command Search and Execution"* — too vague to act
  on as written, so this was actually checked against the spec text
  (`doc/posix/ieee-p1003.2-d11.2-s3.txt:3148`). The core precedence order
  (`exec_search.c`: special builtins → functions → regular builtins →
  `PATH` search) matches the spec. But testing the spec's explicit caveat
  — "an implementation may remember [a command's] location ... unless the
  `PATH` variable has been the subject of an assignment" — found a real
  gap: reassigning `PATH` does *not* invalidate shish's command-location
  cache. Confirmed live against bash with two same-named commands in two
  different `PATH` directories. Precise version now in `BUGS`.
  A second search gap found later (2026-07-21), now fixed: the `PATH`
  walk (`exec_path()`) and `exec_hash()`'s direct-path branch both
  accepted a *directory* as a command match (`access(X_OK)` alone
  doesn't exclude directories) -- fixed by adding an `S_ISDIR` check
  after `access()` succeeds in both places. Exec failure statuses and
  messages were also wrong on both sides of the exec path: the parent
  (lookup failing before any fork) always exited 1 instead of 127/126,
  with the wrong `strerror()` text besides, because nothing captured
  the `access()`/`stat()` errno before later syscalls clobbered plain
  `errno` -- fixed via a new `exec_lasterrno`. The child side (execve()
  itself failing post-fork on something that passed every pre-check,
  e.g. `ENOEXEC`) turned out to flat-out SEGV handling its own error --
  `fdtable_exec()` pops the source stack in preparation for a
  successful exec, and the failure path's error message
  (`source_msg()`) dereferenced the now-empty source unconditionally.
  All of the above now fixed; see `BUGS` for the full writeup.

**Found along the way, not in the original `TODO` at all:** backgrounding
a non-simple (compound) command — `eval_tree.c`'s bgnd handling caused
duplicated execution and corrupted subsequent parsing. The backwards
parent/child branch was flipped in c039280e, but that turned out not to
be the whole bug: the parent still fell through into an unconditional
second `eval_node()` and synchronously `job_wait()`ed the "background"
job. **Fixed (2026-07-21):** reworked to mirror how a backgrounded
*simple* command already behaves (`eval_simple_command.c`'s `X_NOWAIT`
path) — fork, print `"[N] pid"`, and return immediately without a
second evaluation or a blocking wait. While re-testing this, found (but
did not fix, out of scope here) that a backgrounded command followed by
more commands on the *same input line* is separately broken at the
parser level regardless of this fix — see `BUGS`.

**Still open, kept in `TODO`:** the line-editing/terminal-abstraction/
key-bindings rewrite (a design-sized project, not a fixable bug) and
"glob check on subargs other than argstr" (looks like it may already be
handled via `expand_arg.c`'s `S_GLOB` check, but wasn't confirmed either
way with a repro).
