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

   - **Update (2026-07-22):** `./configure --enable-maintainer-mode`
     now runs to completion end-to-end (past the point described
     above) and writes `Makefile`/`config.mk`/`build.mk`. Two more bugs
     found and fixed getting here, neither previously listed: `$$`
     changing value across every pipeline/subshell/cmdsubst fork
     instead of staying fixed for the whole script (`fixes/68`), which
     broke `config.status`'s `conf$$subs.awk` temp-file scheme; and a
     variable/substitution value's backslashes getting one level of
     escaping stripped when re-substituted as a command argument
     (`fixes/69`), which corrupted the `sed` script `config.status`
     builds in `$ac_script` to derive `DEFS` from `confdefs.h`. Two new
     issues surfaced along the way, both in `BUGS`:
     `assign-cmdsubst-value-loses-escaping` (a narrower variant of the
     fixes/69 bug that fixes/69 doesn't cover) and
     `configure-summary-test-invalid-expression` (harmless as far as
     tested, but unisolated). Next step is still the same "run it,
     read config.log, fix the next failure" loop — the two most
     concrete unstarted items are `job-fork-nproc-oob` and the
     `AC_PROG_CC` false negative referenced above.

   - **Update (2026-07-22, later same day):**
     `assign-cmdsubst-value-loses-escaping` fixed (`fixes/70`) — an
     assignment's own "NAME=" text is literal source text sharing one
     argument buffer with its value, and fixes/69's single deferred
     unescape pass over the whole buffer couldn't tell that literal
     prefix apart from an adjacent substitution chunk once the two
     were concatenated. Fixed by having the non-splitting expand_cat()
     branch (what every assignment routes through) unescape each
     literal chunk immediately, before it touches the shared buffer,
     tracked via a new `X_UNESCAPED` flag so `expand_args()` (which can
     still mix a self-corrected quoted chunk with a not-yet-processed
     unquoted one in one command argument) doesn't run its own pass a
     second time over already-final bytes. Verifying this surfaced yet
     another, unrelated, still-open bug, now in `BUGS`:
     `heredoc-body-loses-escaping` — a quoted-delimiter here-document
     body loses one level of backslash escaping before it ever reaches
     a reader, confirmed via `od -c` on the receiving end of a pipe (a
     different subsystem, `src/redir/`, not argument expansion).

   - **Update (2026-07-22, still later):** `heredoc-body-loses-escaping`
     fixed (`fixes/71`) — turned out to be the *same* subsystem after
     all (`expand_copysa()`/`expand_arg()`, not `src/redir/`): a
     heredoc body's `N_ARGSTR` chunk was indistinguishable, at the
     expand-time flag level, from ordinary quoted/unquoted literal
     text, so it got the same `expand_unescape()` pass that undoes the
     parser's glob-protection doubling — even though `parse_here.c`'s
     underlying `parse_squoted()`/`parse_dquoted()` calls skip that
     doubling entirely for heredoc content (never pathname-expanded,
     nothing to protect), so the pass had nothing correct left to do
     but strip a real backslash. Fixed by tagging every heredoc-body
     chunk with a new `S_HEREDOC` flag at parse time and having
     `expand_arg()` leave `X_LITERAL` off for it.

   - **Update (2026-07-22):**
     `configure-summary-test-invalid-expression` fixed (`fixes/72`,
     user-diagnosed) — root cause: `test`/`[` with exactly one
     argument must always just check whether it's non-null per POSIX's
     argument-count table, even when that argument starts with "-" and
     isn't (or even *is*, but lacks its operand) a recognized unary
     operator letter. `test_unary()` in `src/builtin/builtin_test.c`
     tried real unary-operator parsing on any "-..."-shaped single
     argument regardless of whether an operand followed, so
     `test "$LIBS"` with `LIBS=-lm` (the configure summary loop's
     `if test "$LIBS"; then ... fi`) fell through to "invalid
     expression" instead of true. Fixed by only attempting
     operator parsing when at least one more argument remains.

   - **Update (2026-07-23):** `fdtable-cycle-detection` closed
     (`fixes/73`). Extensive fuzzing (fd rotations of various lengths,
     the classic `3>&1 1>&2 2>&3 3>&-` stdout/stderr swap, pipelines
     combined with dup redirections, and the exact illustrative
     example this `BUGS` entry carried) never produced a real infinite
     cycle: `fd_dup()` always flattens a fresh redirection's target to
     its ultimate, already-resolved ancestor at setup time, and every
     redirection clause replaces its slot's occupant with a brand new
     struct, so the dependency graph `fdtable_resolve()` et al. walk
     can't actually cycle through ordinary syntax. Confirmed the
     illustrative example (and the classic swap) resolve correctly via
     new regression tests. Replaced the resolver's defense (a raw
     recursion-depth counter capped at `FDTABLE_SIZE`/`FD_MAX`, e.g.
     1024 — deep enough to risk a real stack overflow before ever
     being reached, and unable to distinguish a genuine cycle from a
     merely long chain) with real graph-cycle detection: a stack of
     the fd numbers actively being resolved on the current call chain,
     checked before each recursive call. Verified against the full
     `tests/posix`/`tests/yash` conformance corpus too (same 75/119
     pre-existing failures before and after — an unrelated, already
     known, unstarted-triage baseline, not something this touched).

   - **Update (2026-07-23, later):** `expr`'s `STRING : PATTERN` match
     operator implemented (`fixes/78`, user-requested) — the classic
     autoconf/libtool idiom `expr conftest.o : '.*\.\(.*\)'` (strip a
     file extension) now works, along with the general BRE `:` match
     operator (literals, `.`, `*`, bracket expressions with ranges/
     negation/`[:class:]`, `^`/`$` anchors, up to 9 `\( \)` capture
     groups). `builtin_expr.c` previously only implemented arithmetic
     (via the same tokenizer `$((...))` uses) plus `length`/`index`;
     `:` was entirely unhandled. Implemented as a small, self-contained
     backtracking BRE matcher (no libc `<regex.h>` — consistent with
     this project's general avoidance of libc facilities, and several
     cross-build targets, e.g. dietlibc, either lack POSIX regex or
     only have a stripped one). Cross-checked 20 cases against real GNU
     `expr` (output and exit status both) with no mismatches. `expr`
     moved from `EXTRA_BUILTINS` to `MINIMAL_BUILTINS`
     (`cmake/Builtins.cmake`) so it's actually enabled (and testable)
     by default, now that it does something real. New
     `tests/builtin-expr.sh` (17 cases).

     Found and fixed a real, separate, unrelated bug while testing
     this: `sh -c command_string [command_name [args...]]` never
     consumed `command_name` into `$0` — it left `$0` as the shish
     binary's own path and every real argument ended up shifted by
     one, with `command_name` itself leaking in as `$1`
     (`dash-c-argv0-not-consumed`, `fixes/77`). Verified against
     bash/dash. No dedicated regression test added for this one — the
     test harness has no portable way to invoke `shish -c` recursively
     from inside a `tests/*.sh` file (no exposed path to the testee);
     verified manually instead.

   - **Update (2026-07-23, incidental):** re-enabled the
     `tests/posix`/`tests/yash` conformance suites in `CMakeLists.txt`
     (they'd been commented out since `tests/posix/fnmatch-p.tst` used
     to hang the testee — fixed since, by `fixes/67`). Confirmed the
     full 239-file corpus now completes in about a minute with no
     hangs from `ctest` itself; found one new one, though:
     `tests/yash/random-y.tst` hangs and hits its own 120s per-file
     `TIMEOUT`, logged as `yash-random-y-tst-hangs` in `BUGS` — bounded
     by that timeout, so it doesn't block `ctest` from terminating, and
     wasn't a reason to leave the suite disabled.

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
   - **Update (2026-07-23):** triaged `tests/posix/fnmatch-p.tst`'s
     `case` failures, logged as `case-pattern-bracket-quote-stripping`
     in `BUGS` ("looked like" a parser bug — a quoted bracket-expression
     `case` pattern matching nothing at all). Root cause turned out to
     be much broader than the name suggested and unrelated to
     quoting/brackets specifically: `eval_case.c` passed `SH_FNM_PERIOD`
     to `path_fnmatch()`, the flag that makes pathname *globbing* hide a
     leading `.` from wildcards — but `case` pattern matching (POSIX
     2.13.1) has no such rule, so *any* `case` statement whose
     scrutinee started with `.` failed to match `*`/`?`/a bracket
     expression at all, including the universal `*` fallback (bracket
     expressions with quoting inside just happened to be what the
     failing test used). Fixed (`fixes/81`) by dropping the flag
     (`path_fnmatch(..., 0)`); confirmed dotfile *globbing* is
     unaffected (separate code path, `expand_glob.c`, untouched).
     Verified against bash/dash. Removing this bug's masking effect
     surfaced one further, genuinely distinct bug in the same test
     block, now logged as `case-quoted-bracket-not-literal`: a fully
     quoted bracket-expression-shaped pattern (`"[.]"`, all three chars
     quoted, unlike `[".]"` where only the char inside is quoted)
     should reduce to the literal 3-character string `[.]` after quote
     removal, but is still treated as a live bracket expression --
     since **fixed (2026-07-23, `fixes/82`)**. Two changes were
     needed together, since the underlying representation (shish
     reuses a literal backslash as its own "protect this char from
     glob reinterpretation" marker) is ambiguous on its own: (1)
     `expand_cat()`'s non-splitting branch unconditionally stripped
     that protective backslash (`expand_unescape()`) before a
     pattern-bound chunk ever reached `path_fnmatch()`, destroying the
     very marker that should have told it `[`/`]` were quoted-literal,
     not real delimiters. Added a new `X_PATTERN` expand flag
     (`expand.h`) that `eval_case.c` now passes to suppress that strip
     for pattern-matching contexts specifically (a plain string value
     still gets unescaped as before). (2) That alone regressed two
     *already-passing* cases in the same test block (`[\]]`/`["]"]`
     matching a literal `]`) — the surviving backslash, now correctly
     preserved outside brackets, was misread by `path_fnmatch()`'s
     bracket-parsing loop as a literal member character when it
     landed *inside* an open bracket expression (real POSIX bracket
     expressions have no escape mechanism, unlike top-level pattern
     text, where a backslash is already handled), so an escaped `]`
     both wrongly counted as a class member on its own AND no longer
     reliably closed the class. Extended `path_fnmatch()`
     (`lib/path/path_fnmatch.c`) to also treat a backslash inside a
     bracket expression as escaping the next char to always be a
     literal member — never a closing `]`, negation, range dash, or
     `[:class:]`-style construct opener — in both the member-matching
     loop and the post-match skip-to-`]` loop. Confirmed all of: the
     new fully-quoted case, the original quoted-inside-brackets case,
     both previously-passing escaped-`]` cases, unquoted ranges, and
     `[:class:]` classes still match correctly; dotfile globbing
     (`expand_glob.c`, a separate code path using real `glob()`, not
     `path_fnmatch()`) is untouched. `tests/posix/fnmatch-p.tst`'s
     "brackets and quotations" block now fully passes (was 4/7 in the
     file, now 6/7 — the file's remaining two failures are the
     unrelated, already-logged `fnmatch-bracket-collating-range` and a
     pre-existing parse error in its unrelated "quotations of
     quotations" block). Found and logged a further, still-open,
     narrower bug while fixing this one: `expand-param-pattern-
     leading-dot` — `expand_param.c`'s `${var#pattern}` family passes
     the same wrong `SH_FNM_PERIOD` flag `eval_case.c` used to.
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
   - ~~Still open, not attempted: no `kill` builtin (signals are only
     reachable from fg/bg's own internal `SIGCONT`, not from scripts
     generally).~~ **Done (2026-07-23).** Added `src/builtin/builtin_kill.c`
     (`kill [-signal|-number] pid|%job ...`), the first real caller of
     `sig_number()` (see item 8 below and `fixes/74`) — signal names
     are looked up case-insensitively with or without a "SIG" prefix,
     falling back to a plain integer for `-9`-style operands. `%job`
     operands resolve via the existing `job_find()`/`struct job.pgrp`
     and signal the whole process group via `killpg()`, same as
     `fg`/`bg`'s own `SIGCONT`. Found and logged a new, unrelated
     parser bug while testing it: `kill-arg-redirect-parse`, since
     **fixed (2026-07-23, `fixes/79`)** — `parse_unquoted.c`'s
     redirection-prefix check ran `scan_uint()` over the parser's
     reused scratch stralloc buffer without nul-terminating it first,
     so a short numeric prefix reusing a buffer that previously held a
     longer, digit-ending token (e.g. `kill`'s own `-0`/`-9`-shaped
     signal arguments) read into stale trailing bytes and silently
     failed to recognize the redirection at all.
   - Also extended (2026-07-23, `fixes/75`, user-requested):
     `src/builtin/builtin_hash.c` was a read-only table dump with no
     argument handling at all. Added `-r` (forget every entry, a real
     free rather than the private, PATH-staleness-driven
     `exec_hash_invalidate_all()` in `exec_hash.c`, which only
     distrusts entries for a re-search, not remove them), `-l`
     (reprint every `H_PROGRAM` entry as a `hash -p pathname name`
     line, directly reusable as input), `-p pathname name` (remember
     an arbitrary name → pathname mapping without searching `PATH`),
     `-d name` (forget one entry), and bare `name` operands (hash
     without running). Caught one bug of its own before it ever
     shipped: `exec_lookup()` only writes back through its `hashptr`
     out-param on a cache *hit* — `-p`'s insert path needs the hash on
     a *miss* (the common case, hashing a name for the first time),
     so it has to be precomputed via `exec_hashstr()` rather than left
     for `exec_lookup()` to maybe fill in, or `exec_create()` buckets
     the new entry by a garbage value nothing can ever look up again.

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

8. ~~Prune dead code, mostly in `lib/sig`~~ **Done (2026-07-23,
   `fixes/76`, user-requested), for `lib/sig` at least.** `builtin_trap.c`
   rewritten to install/remove real-signal traps via `sig_push()`/
   `sig_pop()` (a per-signal save/restore stack, see `sig_stack.c`)
   instead of raw `signal()`/`SIG_DFL` -- `sig_pop()` now correctly
   restores whatever disposition the signal actually had before
   (`SIG_DFL` the first time, matching the old behavior, but the right
   thing on a *nested* retrap too), not just a blanket default. Wiring
   `sig_push()` up for real surfaced two bugs of its own, both fixed:
   `sig_push.c` reinterpreted `sigset_t` (a 128-byte struct on glibc)
   through an `unsigned long*`, undefined behavior that was harmless
   only because the function was unused -- it meant to write the
   `SA_MASKALL`/`SA_NOCLDSTOP` flags into `sa_flags`, not stomp on
   `sa_mask` -- and `trap_install()` never removed the previous trap
   for a signal before installing a new one, so retrapping the same
   signal N times leaked N-1 list nodes forever (pre-existing) and, new
   with a real per-signal stack in the picture, would have exhausted
   `sig_push()`'s fixed 16-deep stack (`SIGSTACKSIZE`) after the 16th
   retrap. Fixed by having `trap_install()` call `trap_uninstall()`
   first. Verified retrapping the same signal 30 times still works. Also
   fixed the `sig.h` double `SA_MASKALL`/`SA_NOCLDSTOP` definitions
   noted below.

   Removed everything left unused after the rewrite: `sig_restoreto`,
   `sig_shield`/`sig_unshield`, `sig_ignore`, `sig_pause`, `sigfpe`,
   `sigsegv`, `sig_blockset`, `sig_dfl`/`sig_ign` (and the `sig_restore`
   macro built on them, likewise never called), `SIG_ALL`, and the
   `sig_addset`/`sig_delset`/`sig_fillset`/`sig_emptyset`/`sig_ismember`/
   `sig_bit` macros (only `sig_restoreto` used any of those, and it's
   gone too). Kept `sigset_type` (used by `struct proc` in `job.h`) and
   `sig_catcha` (turned out to still be needed internally by
   `sig_catch.c`, missed on the first pass). `src/job`'s `job_get`,
   `job_proc`/`proc_bypid` are a separate, still-open item -- not
   touched here.

   Verifying this surfaced a real, unrelated, deep pre-existing bug
   (confirmed present with `sig_push()`/`sig_pop()` stashed out too),
   logged as `subshell-exit-trap-output-misdirected` -- since **fixed
   (2026-07-23, `fixes/80`)**. Two independent bugs turned out to be
   bundled under that one symptom: (1) `eval_subshell()` doesn't fork
   (it isolates a subshell's vars/functions/fds via push-and-pop
   instead, see `exec_functions_save`/`restore`), but never did the
   same for the global `traps` list -- a trap installed inside `(...)`
   stayed installed after the subshell "returned" and only fired later
   on the *parent's* own eventual exit, with the parent's fdstack/
   stdout in effect. Fixed with the same snapshot/restore pattern as
   functions (`trap_snapshot_save`/`_restore` in `builtin_trap.c`,
   called from `eval_subshell.c`; `trap_uninstall()` now also defers
   freeing a removed/replaced node while `exec_subshell_depth` is
   nonzero, exactly like `eval_function.c` already does for redefined
   functions, so a snapshot's saved pointers can't dangle). (2) a
   subshell ending via an explicit `exit` double-fired its own EXIT
   trap: `sh_exit()` unconditionally called `trap_exit()` directly
   *and* called `eval_exit()`, which for a subshell frame *also* runs
   the same trap through the frame's destructor callback before its
   `longjmp`. Fixed by reordering `sh_exit()` to try `eval_exit()`
   first (which only returns without jumping when there's no subshell
   boundary to unwind to, i.e. this really is the outermost shell
   exiting) and only fall through to the direct `trap_exit()` call in
   that top-level case. Separately, falling off the end of a subshell
   body *without* an explicit `exit` never ran the destructor at all
   (only the jump path did) -- `eval_subshell()` now also invokes it
   for that path, while the subshell's own fdstack/vartab/sh context is
   still current, matching bash/dash's observed behavior for all of:
   normal fall-through, explicit `exit`, a real-signal trap installed
   in a subshell (confirmed not to leak into the parent's disposition
   either), and an outer trap left untouched by a same-named one set
   and already fired inside a nested subshell.

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

## Goal 4 — arena allocator for `lib/arena.h`/`lib/arena/` (planned, not started)

**Not implemented yet — this section is a plan to come back to, written
down per explicit request rather than acted on.** The trigger: `src/tree.h`'s
AST is a textbook arena candidate that currently isn't one.

- **Why the tree specifically:** every node is `tree_newnode()`'d
  one-at-a-time via `lib/alloc.h`'s `alloc()` (a thin `malloc()` wrapper;
  see `tree/tree_newnode.c`, sized per-`kind` from the `tree_nodesizes[]`
  table in `tree/tree_nodesizes.c`), and freed the same way, one node at
  a time, via `tree_free()`'s recursive walk (`tree/tree_free.c`) —
  matching `malloc()`/`free()` call for call. But an AST's actual
  lifetime is coarse and all-or-nothing: `sh_loop.c` parses one full
  statement into a tree, evaluates it, then `tree_free()`s the *entire*
  thing and moves on to the next line; nothing ever frees or re-links a
  single node in isolation without the rest of its subtree. That's
  exactly the shape an arena (bump-allocate as you go, free the whole
  region in one call when the *owning* tree goes away) turns from O(nodes)
  malloc+free pairs into O(1) region create + O(1) region destroy, and
  incidentally improves locality (`tree_free()`'s recursive walk over
  scattered `malloc()` chunks becomes irrelevant — nothing to walk).
  `grep -rn "tree_newnode(" src/ --include=*.c | wc -l` currently shows
  32 call sites across the parser/eval/expand layers; every one of them
  is a candidate carried along for free once the underlying node
  allocator changes.

- **Where else it'd help, roughly in order of how cleanly the lifetime
  matches "allocate a batch, discard the batch":**
  1. The AST itself (above) — highest leverage, cleanest lifetime match.
  2. `expand_arg()`/`expand_cat()`'s intermediate `N_ARG`/`N_ARGSTR` chains
     built during word expansion (`src/expand/`) — these are constructed
     fresh per command, consumed immediately (`expand_argv()`,
     `expand_catsa()`, etc.), then thrown away; same shape as the AST,
     smaller scale.
  3. `struct trap_s` in `src/builtin/builtin_trap.c` (fixes/80's
     `trap_snapshot_save()`/`_restore()` already leans on "a subshell's
     nodes get bulk-discarded on restore, not individually freed" as a
     *manual* version of exactly this pattern — see the
     `exec_subshell_depth` guard in `trap_uninstall()`) and the parallel
     `functions` list snapshot in `exec_search.c` are two more places
     already doing informal, hand-rolled "bulk free later" bookkeeping
     that a real arena would subsume more safely (right now both leak
     deliberately between snapshot and restore as their safety margin —
     an arena turns that deliberate leak into an actual, bounded-lifetime
     free).
  4. General scratch allocation during a single eval (`struct eval`
     frames already stack-allocate per-call, not heap — lower priority,
     already fine).

- **API sketch (not committed to — decide at implementation time):**
  `lib/arena.h` declaring `struct arena` (a chain of growable blocks) and
  `arena_alloc(struct arena*, size_t)` (bump-allocate, grow by appending a
  new block on overflow, no per-call `free()`), `arena_reset()` (rewind
  to empty without releasing the underlying blocks, for reusing one arena
  across many statements in the interactive loop — the common case) and
  `arena_free()` (release every block back to the system). `lib/arena/`
  holds the `.c` files (one function per file, matching every other
  `lib/*/` module's convention) plus a `Makefile.in`/CMake `file(GLOB)`
  pickup, same as `lib/sig/`, `lib/stralloc/`, etc.
  `tree_newnode()` would grow a `struct arena*` parameter (probably
  threaded through `struct parser`, since that's what currently owns a
  tree's lifetime start-to-end) and `tree_free()` would shrink to a
  single `arena_reset()`/`arena_free()` call at the tree's root instead
  of a recursive per-node walk.

- **The three open questions above have now been investigated
  (2026-07-23) — still not implemented, but each one has an answer or a
  concrete plan instead of a question mark:**

  1. **`DEBUG_ALLOC`'s existing debug story: there wasn't one to
     preserve, and the option is now gone entirely.** Confirmed by
     actually building it (`cmake -S . -B /tmp/b -DDEBUG_ALLOC=ON &&
     cmake --build /tmp/b`): it failed at the *link* step.
     `debug_alloc()`/`debug_free()`/`debug_realloc()`/`debug_heap` were
     declared (`src/debug.h`) and called from every `alloc()`/
     `alloc_free()` call site once the option was on, but
     `CMakeLists.txt` listed three source files for them
     (`lib/alloc/debug_alloc.c`, `debug_free.c`, `debug_realloc.c`) that
     didn't exist anywhere in the tree — only `debug_error.c` did. This
     had apparently been broken for a while (not something this
     session's other changes touched). **Removed entirely
     (2026-07-23)**, per explicit request, rather than left broken: the
     option, every `*debug.c` file it gated in `lib/alloc/`/
     `lib/stralloc/` (`allocdebug.c`, `alloc_zerodebug.c`,
     `alloc_redebug.c`, `str_dupdebug.c`, `debug_error.c` — orphaned,
     never called outside its own gated definition —
     `stralloc_freedebug.c`/`readydebug.c`/`readyplusdebug.c`/
     `truncdebug.c`), `parse_newnodedebug.c`/`tree_newnodedebug.c`,
     `src/debug/debug_memory.c`, the `dump` builtin's now-gone `-m`
     option, and every `#ifdef`/`#ifndef DEBUG_ALLOC` guard around the
     surviving plain implementations. Verified via an A/B `git stash`
     comparison that the same 4 pre-existing flaky test failures
     (fd/pipeline-related, environment resource pressure — `/mnt/data`
     is at 100% full and ~28 stray `shish` processes from old hung
     `yash-random-y-tst-hangs` runs are still lingering) occur
     identically with and without this removal, i.e. no regression.
     Consequence for this plan: there's no working per-allocation
     tracking to match or preserve, and no such tracking exists in the
     tree to design around anymore either — the arena's own debug/
     leak-hunting story, if wanted, gets designed from scratch.

  2. **A single arena is *not* enough, but the fix is a plain stack of
     them, not anything fancier.** `grep -rn "tree_free(" src/` (23
     call sites, excluding `tree/tree_free.c` itself) sorts into two
     kinds: most are ordinary nested cleanup of a subtree that's still
     part of the *current* enclosing statement's tree (aborted partial
     parses in `parse_expect.c`/`parse_word.c`/`parse_here.c`/
     `parse_arith_binary.c`/`parse_arith_paren.c`; transient `N_ARG`
     chains fully consumed within one eval call in
     `eval_simple_command.c`/`eval_for.c`/`expand_param.c`/
     `expand_str.c`) — these need no special handling at all under an
     arena: they'd just become no-ops, leaving the now-dead nodes to be
     reclaimed whenever the *enclosing* scope's arena eventually resets,
     which is strictly a small, one-time-per-statement memory-locality
     cost, not a correctness concern. The other kind is a short list of
     genuinely **independent** parse-evaluate-free scopes, each with its
     own dedicated `parse_init()` + matching `tree_free()`, confirmed to
     nest strictly via ordinary C call-stack recursion and never run
     concurrently (shish is single-threaded and synchronous, so there is
     no scenario where two such scopes are simultaneously "open" without
     one being nested inside the other): the top-level loop
     (`sh_loop.c`), `builtin_eval.c`, `builtin_source.c` (which just
     calls `sh_loop()` recursively — same scope shape one level deeper,
     via `source_push()`), `builtin_expr.c`'s arithmetic-string parse,
     `prompt_parse.c`'s `PS1`-string parse, and `builtin_trap.c`'s own
     inline parse of a trap's command string. A plain **stack of
     arenas** — push one on entering any of these, pop (and release) it
     at the matching `tree_free()` — covers every one of them with no
     need for reference counting, a tree of arenas, or anything
     per-subshell specifically (a subshell doesn't get its own parse
     scope at all; `eval_subshell()` evaluates a subtree that was
     already part of its parent statement's single parse, so it just
     rides along in whatever arena is already on top of the stack).

  3. **The real obstacle isn't `tree_free()`'s non-node-memory frees —
     it's `eval_function()`'s ownership-transfer trick, and only that.**
     Full enumeration of `tree_free()`'s frees beyond the node struct
     itself: `N_ARG`/`N_ASSIGN` and `N_ARGSTR`'s own `stralloc` buffers,
     `N_FOR`'s `varn` string, `N_ARGPARAM`'s `name` string, and
     `N_FUNCTION`'s `name` string. All five are variable-length
     buffers that grow via `realloc()` (stralloc) or are sized exactly
     once at parse time (the plain strings) — neither shape fits a bump
     allocator cleanly, so the plan going in is: **the arena covers only
     the fixed-size `union node` structs**, and every string/`stralloc`
     payload a node carries stays on the ordinary heap exactly as today
     (still individually `alloc()`/`alloc_free()`'d). This alone doesn't
     block anything; nodes are the majority of the allocation *count*
     even where strings dominate total *bytes*.
     The actual hard case, reading `eval_function.c` closely: redefining
     or first-defining a function doesn't reuse the already-parsed
     `N_FUNCTION` node from the current statement's tree at all — it
     allocates a **brand new** one, steals that node's `name`/`body`
     pointers onto it (`fn->nfunc.name = func->name; fn->nfunc.body =
     func->body;`), and nulls out the original's fields
     (`func->name = func->body = NULL;`) so the enclosing tree's own
     `tree_free()` — which null-checks both fields — skips them and
     doesn't cascade into what's now owned by the long-lived, global
     `functions` list instead. That body subtree, though, was allocated
     *as part of* the enclosing statement's own transient parse — under
     a "reset the whole arena once this statement's `tree_free()` runs"
     scheme, the adopted body would be sitting on memory that's about
     to be invalidated. Trap bodies don't have this problem (see #2:
     `builtin_trap.c` parses a trap's command string through its own
     fully independent `parse_init()` call already, so it can simply get
     its own dedicated arena that's never reset until the trap is
     replaced/removed — no adoption trick needed at all), but function
     bodies parse inline as an ordinary part of whatever statement
     defines them, so there's no separate parse scope to redirect ahead
     of time. Two ways to resolve this, to decide between at
     implementation time rather than now: **(a)** deep-copy the adopted
     body subtree out of the transient arena into long-lived storage at
     the exact point `eval_function()` currently does the pointer-steal
     (correctness-preserving, only pays a copy once per function
     definition/redefinition, which is rare), or **(b)** have the parser
     itself detect "currently parsing a function body" and switch which
     allocator `tree_newnode()` draws from for just that subtree's
     duration, so it's never in the transient arena to begin with.
     `builtin_unset.c`'s own `tree_free(fn)` (removing a function) is the
     same long-lived-list shape from the other end and corroborates this
     — a function's node lifetime is tied to the `functions` list, never
     to any statement's parse scope.

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
key-bindings rewrite (a design-sized project, not a fixable bug).
"glob check on subargs other than argstr" turned out to be three
separate, compounding bugs once confirmed with a repro -- all now
**fixed** (`fixes/66-glob-not-triggered-for-plain-arguments.patch`):
`expand_cat.c` unconditionally re-escaped every glob-special char via
`expand_escape()` (removed entirely, along with the dead, never-set
`X_ESCAPE` flag it existed for) before `expand_glob()` ever saw it;
`parse_unquoted()` lost its locally-tracked `S_GLOB` flag whenever the
pattern was the last thing before end-of-input (the same root cause
as `dash-c-for-loop-parse-error`, fixes/56, had for keyword
recognition); and `expand_glob()`'s own result-building loop left the
first match's node unterminated at its new, shorter length, leaking
the original pattern's own leftover bytes into the output for some
match shapes.
