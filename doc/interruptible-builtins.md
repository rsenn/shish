# Interruptible in-process builtins (research, no design chosen yet)

Status: **inquiry, not a plan.** Nothing below is scheduled or half-committed to;
it's a survey of the problem, the codebase machinery already available, and the
routes open, written down before picking one. Keep routes open until a decision
is made.

## 1. The problem, demonstrated

`sleep 30` (a builtin, `src/builtin/core/builtin_sleep.c`) run at an interactive
prompt, **not backgrounded**, cannot be interrupted at all:

```
$ sleep 30
^C                          <- typed at second 2; nothing happens
                             <- shell returns at second 30, not before
```

Verified with a pty driver (same technique as `tests/term-complete.sh`): sending
`\x03` (Ctrl-C) while `sleep 30` is running produces no visible effect in the
shell's output beyond the terminal's own local echo of `^C` -- the command runs
to completion regardless.

This isn't `sleep`-specific. It's true of every builtin that runs **in-process**
(the vast majority: all of `src/builtin/extra/*` on the filter framework --
`cat`, `head`, `uniq`, `cut`, `paste`, `nl`, `tr`, `sed`, `grep`, `compress` --
plus `read`, `wait`, `tee`, and anything else that doesn't fork). Only an
**external program**, run via `fork()`+`exec()`, is currently interruptible,
because the terminal delivers `SIGINT` to the whole foreground process group,
which includes that forked child but (see below) never really "includes" shish
itself in a way that does anything.

## 2. Root cause

`sh_sigignore()` (`src/sh/sh_main.c:390`, called once at shell startup for an
interactive session) sets `SIGINT`/`SIGQUIT`/`SIGTERM` to `SIG_IGN` **for the
shell's own process**, and this is never toggled back while the shell is
running a foreground in-process builtin:

```c
/* src/sh/sh_sigignore.c */
void
sh_sigignore(void) {
  ...
  for(i = 0; i < sizeof(sh_sigignore_list) / sizeof(sh_sigignore_list[0]); i++) {
    ...
    sh_sigset(sh_sigignore_list[i], SIG_IGN);
  }
}
```

`sh_sigrestore()` -- the function that puts these signals back to `SIG_DFL` --
is called from exactly one place: `src/exec/exec_program.c:299`, right before
`exec()`ing an **external** program in a forked child. A builtin never forks,
so it never goes through this path; `SIGINT` stays ignored at the process level
for as long as the builtin runs, full stop. There is currently no code path
that makes an in-process builtin's own blocking work observe `SIGINT` at all.

A second, independent wrinkle: even if `SIGINT` were deliverable, several of
`lib/buffer`'s own low-level I/O retry loops treat `EINTR` as "try again,
forever," not "something wants my attention":

```c
/* lib/buffer/buffer_stubborn.c, buffer_putflush.c, buffer_stubborn2.c */
if(errno == EINTR)
  continue;   /* retries the write immediately, no way out */
```

So even a builtin whose *syscalls* got interrupted would currently spin right
back into the same blocking call, at least along this code path.

## 3. What "abortable" should probably mean (and what it probably shouldn't)

The phrase "restartable" is worth pulling apart, because it can mean two very
different things:

- **(a) Abort-and-discard** -- Ctrl-C kills the *current command* outright:
  whatever it had built up (partial output already written is fine; internal
  parse/loop state is not) is thrown away, control returns to the prompt, and
  `$?` becomes `128 + SIGINT` (matching what already happens for a forked
  child that dies from `SIGINT` -- see `job_wait.c`'s `WAIT_TERMSIG(s) ==
  SIGINT` handling, which already exists and already produces this exit
  status for external commands).
- **(b) True suspend/resume** -- the builtin's execution state is checkpointed
  so it can later continue from exactly where it left off (a coroutine or
  continuation).

**(a) is what every real shell actually does.** bash, dash, ksh: Ctrl-C during
a builtin or a blocked read unwinds straight back to the top-level read-eval
loop; nothing about the interrupted command is preserved or resumable. `(b)` is
a fundamentally bigger, riskier build (effectively giving every builtin
coroutine semantics) to solve a problem nothing in POSIX or common shell UX
actually asks for -- `Ctrl-Z` (suspend a whole *process*, resumed by the OS's
own job-control signals) already covers the "come back to this later" case for
external commands, and an in-process builtin doesn't have a process of its own
to suspend that way.

This doc still keeps "keep all routes open" literally -- (b) is listed among
the routes below -- but the survey below is written with (a) as the working
assumption for what "interrupted" produces, since it's the only one with real
precedent, both in other shells and in this codebase's own exit-status
handling.

## 4. Machinery already in this codebase worth building on

shish already has a working async-signal-safe dispatch pattern, used today for
`SIGCHLD` and for any signal with a user-installed `trap`. It's the natural
place to look first, whichever route gets picked.

**The self-pipe (`job_sigfd`)** (`src/job/job_init.c`, `src/job.h:69`): a
`pipe()` whose write end a signal handler can touch (a `write()` of one byte is
async-signal-safe; buffered I/O is not), and whose read end is `select()`ed on
by `term_jobwait()` (`src/term/term_read.c`) and drained from ordinary
(non-signal) context. This is exactly the primitive needed for "a signal fired,
go wake up and deal with it later, safely" -- already built, already proven
(fixed as `fixes/87`, "sh-onsig-async-unsafe").

**`trap_relay()` / `trap_run_pending()`** (`src/builtin/builtin_trap.c`): for
any signal the user has `trap`ped, `trap_relay()` (the real `sigaction`
handler) only sets a flag and pokes `job_sigfd[1]`; `trap_run_pending()` (run
later, from safe context) does the actual trap-body work. Two details matter a
lot for this problem specifically:

- `trap_relay()` installs with **`SA_NORESTART`** (`builtin_trap.c:439`,
  explained in its own comment): a blocking syscall the shell is inside when
  the signal fires returns `EINTR` instead of auto-resuming, specifically so
  the pending-trap dispatch can happen *promptly* rather than only at the next
  statement boundary. `job_wait.c:137`'s comment describes this exact
  mechanism keeping a real-signal trap responsive while blocked on `wait4()`
  for a long-running external command.
- `trap_run_pending()` already gets called from more than one place --
  `sh_loop.c` between top-level statements, and `job_wait.c` when a wait's
  underlying syscall is interrupted -- i.e., there's already a notion of
  "checkpoints where pending signal work gets serviced," not just one.

**What's missing today**: none of this fires for a *bare*, un-`trap`ped
`SIGINT` during an in-process builtin, because `sh_sigignore()` never installs
`trap_relay` (or anything else) for it by default -- it installs `SIG_IGN`,
which is a real ignore at the kernel level, not "come back later." And even
where `trap_run_pending()` does run, it only handles the user's own trap
*body*; there's no existing notion of "the currently-running builtin itself
was asked to stop."

## 5. Where the interruption would actually need to land

A survey of what currently blocks in-process, as a catalog for whichever route
gets picked (not exhaustive, but representative of the shapes involved):

| kind | example | shape |
|---|---|---|
| single blocking syscall, no loop | `builtin_sleep.c`: `sleep()`/`usleep()` | nothing to "check between iterations" -- the call itself is the whole wait |
| read loop over input | `filter_drain()` (`src/builtin/builtin_filter.c`), used by every filter-framework builtin (`cat`, `tr`, `sed`, `grep`, ...) | `while(step(...))  buffer_put(...)` -- iterates once per unit; a slow/blocked upstream (a pipe, a fifo, an interactive `cat` with no input yet) blocks inside `step()`'s own read, not in this loop |
| explicit wait loop | `builtin_wait.c` | `while(job_list) ...` -- already has a natural per-iteration point, and already sits next to `job_wait()`'s existing `trap_run_pending()` call |
| read-until-delimiter | `builtin_read.c`, `builtin_tee.c` (`buffer_get_until`) | blocks inside the buffer layer's own `read()`, several calls deep from the builtin |
| low-level retry-on-EINTR | `lib/buffer/buffer_stubborn.c`, `buffer_putflush.c`, `buffer_stubborn2.c` | currently swallow `EINTR` unconditionally; would need to distinguish "interrupted, stop" from "interrupted, resume" |
| external command, already fine | anything via `job_wait()` | already interruptible today (forked child gets real `SIGINT` from the terminal); already produces `128+SIGINT` via `WAIT_TERMSIG` |

## 6. Routes (all still open)

None of these are mutually exclusive with all of the others; a few combine
naturally. Listed as separate routes because each has a distinct risk profile
someone deciding later will want to weigh independently.

### Route A -- `sigsetjmp`/`siglongjmp` unwind

One `sigsetjmp()` catch point (candidate: `sh_loop()`'s per-statement loop, or
lower, around `eval_tree()`/`exec_command()` for a single command). The
`SIGINT` handler -- installed for real (not `SIG_IGN`) whenever a foreground
in-process builtin is about to run -- calls `siglongjmp()` straight back to
that point.

- **Covers every builtin uniformly**, present and future, with zero
  per-builtin code -- the single biggest point in its favor given how many
  builtins there already are and how many more this project's `TODO.md` Goal
  12/16 plan to add.
- **Risk, and it's a real one**: unwinding mid-builtin skips every `free()`,
  `close()`, buffer flush, and partial cleanup between the interrupted point
  and the catch point. This project already treats memory safety as a
  standing gate (`BUGS: asan-leak-residue-not-fully-triaged`, `TODO.md`'s
  "Memory safety" requirement) -- a longjmp-based abort would need every
  builtin's resource handling (and the filter framework's `ctx`/`stralloc`
  ownership in particular, given how much of it this session just built) *re*
  -audited for "what does a mid-flight unwind leave dangling," which is a
  different and probably larger audit than writing the interruption code
  itself.
- Signal-handler safety: `siglongjmp()` out of a signal handler is
  well-defined POSIX behavior (unlike most other work a handler might do), so
  this route doesn't fight the "async-signal-safe handler" constraint the way
  a naive approach might -- but it does mean the *catch point*'s caller-side
  invariants (what state is guaranteed valid there) become load-bearing in a
  way they aren't today.
- Where it would live relative to existing code: `job_fork()`/`exec_program.c`
  already have well-understood signal-disposition save/restore points for the
  forked-child case; this route's shell-side handler would sit next to
  `sh_sigignore()`/`sh_sigrestore()`, not replace them (external commands
  still work the way they do today).

### Route B -- cooperative flag check (extends the `trap_relay` pattern)

A real handler for un-`trap`ped `SIGINT` (installed the same way
`trap_relay()` already is for a user trap, with the same `SA_NORESTART`) does
nothing but record "interrupted" and poke `job_sigfd[1]` -- exactly
`trap_relay()`'s own shape, reused rather than invented. Every blocking loop
in the catalog above (§5) checks a flag (call it `sh_interrupted` or similar)
at each iteration and bails out on its own terms -- flushing what it's
already produced, freeing what it owns, returning a status the caller can
turn into `128+SIGINT`.

- **No cleanup skipped anywhere** -- every builtin's own code decides how to
  unwind itself, so this doesn't touch the memory-safety story adversely, and
  arguably improves it (an explicit "handle SIGINT" path forces a look at
  what a builtin owns at that point anyway).
- **Touches every long-running loop individually** -- `filter_drain()` is one
  choke point that would cover most of today's builtins in one place (a real
  win, since `TODO.md` Goal 13's whole point was routing builtins through
  shared code), but `builtin_sleep.c`'s single `sleep()`/`usleep()` call has
  nowhere natural to check a flag *inside* the call -- it would need
  restructuring into a loop of short `nanosleep()`s (or a `select()` with a
  timeout) checking the flag between them, and any future builtin that adds
  its own ad-hoc blocking call (not routed through `filter_drain()` or a
  shared helper) would silently miss the coverage unless someone remembers to
  add the check.
- `SA_NORESTART` (as `trap_relay()` already proves) is what makes a syscall
  the shell happens to be blocked in at the moment of the signal return
  `EINTR` promptly rather than transparently resuming -- this route depends on
  that working the same way for a bare `SIGINT`, which is a reasonable bet
  given it's already relied on for real-signal traps today.
- The `lib/buffer` `EINTR`-retry loops (§2) would need to distinguish "this
  `EINTR` was the abort signal, stop" from "this was some other, harmless
  interruption, keep going" -- likely by checking the same flag before
  retrying.

### Route C -- hybrid

Cooperative checks (Route B) in the loops that already naturally iterate
(`filter_drain()`, `builtin_wait.c`'s wait loop, `builtin_read.c`), reserving a
`siglongjmp` catch point (Route A) only as the fallback for the few genuinely
single-shot blocking calls that have no loop to instrument (`sleep()`/
`usleep()` chief among them, until/unless those get rewritten into a
poll-style loop anyway, at which point they'd migrate to a plain Route-B
check and the longjmp fallback would shrink over time rather than grow).

- Smaller blast radius for the "what does an unwind leave dangling" audit
  (§Route A) -- only the calls that actually use the fallback need it, and
  that set is small and enumerable today.
- Still two mechanisms to reason about instead of one, which cuts against the
  "minimum code that solves the problem" bias this codebase's own `CLAUDE.md`
  states -- worth weighing against how much smaller each mechanism's own
  scope becomes.

### Route D -- per-builtin opt-in flag

Add an explicit "this builtin can be interrupted" marker (a bit in
`struct builtin_cmd`, or, for the filter framework specifically, a field on
`struct filter_ops` alongside `output`/`each` from today's session) rather
than making interruptibility a blanket property of "runs in-process." Only
builtins that actually declare themselves interruptible get any of the
mechanism above; everything else keeps running exactly as it does today,
uninterruptible, by design (matches POSIX's own silence on this -- there's no
requirement that *every* builtin be interruptible, only that the shell as a
whole remains responsive).

- Lets Route A/B/C's actual mechanism get proven on one or two builtins
  (`sleep` is the obvious first candidate: one blocking call, well-understood,
  high user-visible payoff) before deciding whether to roll it out further.
- Not really an alternative to A/B/C so much as a rollout strategy that
  composes with any of them -- listed separately because "which builtins get
  this at all, and in what order" is its own open question independent of
  "how does the interruption itself work."

## 7. Open questions

- **Scope of "in-process work" that counts.** A shell function body, a
  `while`/`for` loop written in shell script (not inside a single builtin at
  all), and command substitution `$(...)` all currently run in-process too --
  does "interruptible builtins" implicitly want "interruptible shell script
  execution in general" as well? `sh_loop()`'s per-statement boundary already
  gives that case a natural checkpoint (§4's "already more than one place"
  point) almost for free under Route B/C, which the single-builtin framing of
  this doc's title doesn't fully capture.
- **What "abort" prints/returns for each builtin.** `job_wait.c` already
  squelches the "signaled" banner for a `SIGINT`-killed child in most cases
  (`squelch = ... WAIT_TERMSIG(s) == SIGINT`) -- an aborted in-process builtin
  presumably wants the same restraint, but each builtin's own partial-output
  story (has it already written some stdout? a partially-written `sed -i`-style
  in-place edit? a half-flushed `tee` to a file?) differs enough that "abort
  cleanly" may not mean the same thing everywhere.
- **Nested interruption.** A builtin that itself runs a sub-shell/sub-parse
  (alias expansion's `parse_simple_command()` recursion, command substitution,
  a trap body run via `trap_run_pending()` itself) raises the question of
  which level an interrupt during that nested work should unwind to -- POSIX
  and real shells have answers here (traps mask themselves while running,
  `$(...)` in a pipeline behaves like its own subshell for signal purposes),
  but they'd need mapping onto shish's actual call structure, not assumed.
- **Windows.** `WINDOWS_NATIVE`/`__MINGW64__` builds (`cfg-mingw32`/
  `cfg-mingw64`) don't have POSIX signals in the same shape `sh_sigignore.c`
  already `#if !WINDOWS_NATIVE`-guards around -- whichever route is chosen
  needs its own story there, or an explicit decision that this is
  POSIX-platform-only for now (the existing self-pipe/`trap_relay` machinery
  is already `!WINDOWS_NATIVE`-gated, so there's precedent for scoping it out
  rather than blocking on it).

## 8. Non-goals (for now, revisit if wrong)

- **True suspend/resume (§3(b))** -- no real-shell precedent, no POSIX
  requirement, meaningfully bigger and riskier than (a); listed as Route
  among-routes only for completeness, not because the survey above found a
  reason to prefer it.
- **Interrupting an *external* program more than it already can be** -- that
  path already works (forked child, real terminal `SIGINT`, already produces
  `128+SIGINT` via `job_wait.c`). Out of scope; not broken.
