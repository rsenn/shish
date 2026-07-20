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

4. **Wire `tests/posix/*.tst` (127 files) and `tests/yash/*.tst` (121
   files) into the build.** These are an existing, substantial POSIX/yash
   conformance test corpus (yash's own `.tst` format, with a runner already
   present at `tests/posix/run-test.sh`) that is sitting in the repo
   entirely unreferenced by `CMakeLists.txt` — `ctest` only runs the 11
   ad hoc top-level `tests/*.sh` scripts. Nothing here is currently
   measuring real POSIX coverage; this is the highest-leverage item in this
   goal because it replaces guessing at gaps with a concrete, ranked
   failure list to work through. `run-test.sh` was written for yash and
   uses a couple of yash-isms (`command -b ulimit`) — expect to need a
   small adapter, not a rewrite, to point it at `shish` and register each
   `.tst` file (or a summarized subset) as a CTest case.
5. Once 4 is wired up, prioritize by what it reports — don't hand-pick
   POSIX sections to fix ahead of that data.
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

1. **Fix the `fg` out-of-bounds crash.** `builtin_fg`
   (`src/builtin/builtin_jobs.c`) declares `struct job *joblist[njobs]`
   where `njobs = argc - 1` — **0** for bare `fg` (no arguments, the common
   case) — and only corrects `njobs` to 1 *after* that zero-length VLA is
   already allocated. `joblist[0] = job_current();` then writes past it.
   Reproduced live under ASan:
   ```
   $ sleep 3 & fg
   ==ERROR: AddressSanitizer: dynamic-stack-buffer-overflow ... in builtin_fg src/builtin/builtin_jobs.c:30
   ```
   Highest-leverage item here: it's a crash in the single most common
   invocation of a builtin that ships enabled by default.

2. **`job_fork()` blocks `SIGCHLD` before `fork()` and never unblocks it**,
   in either the parent or child path (`src/job/job_fork.c`). Grepped every
   call site — the only `sig_unblock(SIGCHLD)` in the codebase is inside
   `job_update()`, gated behind `if(job_signaled)`, which can only become
   true by the SIGCHLD handler firing — which can't happen while the signal
   stays blocked. Net effect after the *first* forked job: (a) the shell
   likely stops getting async job-completion notifications from then on,
   and (b) worse, the blocked mask is inherited across `fork`+`exec` into
   every subsequently launched program — anything that expects to catch
   its own children's `SIGCHLD` (`make`, a nested shell) silently gets it
   blocked by shish. Fix: `sig_unblock(SIGCHLD)` on the parent-return path,
   and reset the mask before the child execs. Affects every external
   command, not just job control — second-highest leverage.

3. **`fg`/`bg` don't implement job control's stop/resume half at all.**
   Beyond the crash in #1, `fg` never calls `job_foreground()` (the *only*
   function in the codebase that hands the controlling terminal to a job's
   process group via `tcsetpgrp` — and it is never called from anywhere),
   never sends `SIGCONT`, and waits via a CPU-spinning busy loop
   (`while((r = wait_pids_nohang(...)) > 1) {}`) instead of blocking.
   `builtin_bg` is an empty stub (`return 0;`, nothing else). There is no
   `SIGCONT` anywhere in the codebase and no `kill` builtin, so even
   fixing `fg`/`bg`'s internals, nothing can resume a `Ctrl-Z`-stopped job
   today. Needs: a real `fg` (call `job_foreground()`, send `SIGCONT`,
   block on `job_wait()`), a real `bg` (send `SIGCONT`, leave it running,
   print the `[n] pid` line the same way `exec_program.c`'s `X_NOWAIT`
   path already does), and a minimal `kill` builtin so signals are
   reachable from scripts at all.

4. **`WAIT_EXITSTATUS` (`lib/wait.h`) and the fallback `WEXITSTATUS`
   (`src/job.h`) are both wrong**, same bug in two places: both are
   `(status) & 0x7f`-style masks of the raw wait-status low byte instead
   of `(status >> 8) & 0xff`. For any normally-exited process the low byte
   is 0 by construction, so `WAIT_EXITSTATUS` always evaluates to 0 —
   currently dormant only because its one live call site
   (`job_printstatus`'s "exited" branch) is only reached when `job_wait`
   already determined the process did *not* exit normally; there's a
   commented-out call in `exec_program.c` that looks like a past attempt
   to use it directly, which would immediately hit this. `WEXITSTATUS` is
   the one that matters more: `eval_pipeline.c` uses it to compute `$?`.
   It's masked on Linux/glibc only because `<sys/wait.h>`'s own correct
   macro wins via the `#ifndef WEXITSTATUS` guard — on `WINDOWS_NATIVE`
   builds (no `<sys/wait.h>`), this broken fallback is what's actually
   used, so `$?` for external commands is wrong there today. Fix both to
   `((status) >> 8) & 0xff`.

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

## Known-but-not-yet-actioned (from `BUGS`, still true)

- `while ...; exec 5<>lala` bug (unverified against current code, not
  re-tested during this pass — do so once item 1 above is fixed, they may
  share a root cause in the fdtable).
