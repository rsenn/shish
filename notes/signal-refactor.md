# signal subsystem: summary and simplification strategy

Everything under `grep -rE '\bsig\w*\(' lib src -l` -- 22 files across
`lib/sig/`, `src/builtin/`, `src/job/`, `src/exec/`, `src/term/`, and
`src/sh/`. This is a summary of how it actually works end to end, what
in it is genuinely load-bearing, and what's accumulated redundancy
worth removing -- independent of, but relevant to, the mingw
`sig_action` gap tracked in `mingw-porting.md`.

## The three layers, as they exist today

```
sig_action()            lib/sig/sig_action.c
  translates shish's own flag vocabulary (SA_MASKALL, SA_NOCLDSTOP,
  SA_NORESTART -- lib/sig.h:81-96, none of them real SA_* values) into
  a real sigaction(2) call. #ifdef SA_RESTART-gated: only compiles
  where a real sigaction(2) exists.

sig_push() / sig_catch()   lib/sig/sig_push.c, sig_catch.c
  build a struct sigaction with the flags above and call sig_pusha()
  (below) or sig_action() directly.

sig_pusha() / sig_pop()    lib/sig/sig_stack.c
  a per-signal stack (sig_stack[NSIG-1][SIGSTACKSIZE=16]) of saved
  dispositions. Calls sig_action() to install the new one *and*
  capture the old one into the next stack slot in the same call.

sig_block() / sig_unblock() / sig_blocknone()   lib/sig/sig_block.c etc.
  thin sigprocmask() wrappers, unrelated to the sig_action/stack
  machinery above -- a separate concern (temporarily blocking a
  signal around a critical section, not changing its disposition).

raw libc signal()       src/term/term_init.c, term_restore.c
  a *third*, structurally different style: no mask semantics, no
  stack, bypasses sig_action entirely.
```

Three coexisting styles for "do something about a signal," with no
documented rule for which to reach for. That's the headline finding --
not that any one of them is wrong, but that nothing stops a fourth
call site from picking arbitrarily.

## Traced: what actually happens for the two things this subsystem is for

**`trap 'echo hi' INT` (installing a trap):** `builtin_trap()` ->
`trap_install()` -> `trap_uninstall()` first (always -- see below) ->
`sig_push(SIGINT, trap_relay)` -> builds the internal-flag
`struct sigaction` -> `sig_pusha()` -> bounds-checks against
`NSIG`/`SIGSTACKSIZE`, calls `sig_action()`, saves the old disposition
into the stack -> `sig_action()` -> real `sigfillset()` +
`sigaction(2)`. Six layers from the builtin to the syscall wrapper;
three of them (`sig_push` -> `sig_pusha` -> `sig_action`) are pure
pass-through, no branching of their own beyond flag translation and
bookkeeping.

**The signal actually firing:** kernel -> `trap_relay()` (the real OS
handler -- async-signal-safe: sets a pending flag, writes one byte to
a self-pipe) -> later, from ordinary execution context ->
`trap_run_pending()` -> `trap_handler()` -> `eval_tree()` runs the
trap body. Same self-pipe shape as `sh_onsig()`'s SIGCHLD handling in
`sh_main.c` -- applied consistently, correctly, not something to
simplify away.

**`job_fork()` forking a pipeline member:** `sig_block(SIGCHLD)` (4
libc calls: `sigemptyset`+fetch-`sigprocmask`+`sigaddset`+
`sigprocmask`) -> `fork()` -> child unblocks before `exec()`, parent
unblocks after job-table bookkeeping. Two full round trips through
`sigprocmask()` to protect one `fork()` against a fast-exiting child's
`SIGCHLD` racing the parent's own bookkeeping -- this is the minimum
needed for the race it's guarding against, not an obviously
simplifiable pattern without a bigger redesign (see "not attempted
here," below).

## What's load-bearing -- do not remove in any refactor

- **The `WINDOWS_NATIVE` signal-number remapping** (`lib/sig.h:8-38`,
  `SIGHUP`=1 through `SIGSYS`=31). Lets shish talk about "SIGINT"
  consistently regardless of which subset the host libc defines. Any
  restructuring keeps a project-owned number space.
- **The `SA_MASKALL`/`SA_NOCLDSTOP`/`SA_NORESTART` internal-flag
  vocabulary.** Decouples shish's semantic intent from real OS flag
  values -- exactly what makes one `sig_action()` implementation
  portable across "has real sigaction" and "doesn't." Keep the
  *interface* even where the implementation changes.
- **`sig_stack.c`'s push/pop stack.** Not redundant bookkeeping
  duplicating the kernel -- the kernel only remembers the *current*
  disposition, not a shell-level nesting history, and
  `trap_snapshot_save()`/`trap_snapshot_restore()`
  (`builtin_trap.c:385-468`) need exactly that history to isolate a
  subshell's `trap` from leaking into its parent (`eval_subshell.c`
  doesn't fork, so this can't fall out of process isolation for
  free). `SIGSTACKSIZE=16` is a real bound tied to nested-subshell
  trap-redefinition depth, not an arbitrary constant.
- **The self-pipe pattern** in `trap_relay()`/`sh_onsig()`. Correct,
  consistent, async-signal-safe. Collapsing it into synchronous work
  inside the handler would reintroduce real bugs (allocation/buffered
  I/O inside a signal handler).
- **`trap_install()`'s always-uninstall-first pattern.** Keeps the
  stack from growing on ordinary re-trapping (`trap CMD INT` twice in
  a row pops the first before pushing the second) -- without this,
  a script re-trapping the same signal in a loop would exhaust
  `SIGSTACKSIZE`.

## What's redundant or just accumulated cruft

Ranked by how concrete/mechanical the fix is, not by size of win.

1. **`sig_table.c`'s whole body is `#if !WINDOWS_NATIVE`
   (`sig_table.c:11-43`).** On that platform the name<->number table
   is empty except the `EXIT` sentinel, even though `lib/sig.h`
   defines real Windows signal numbers one file over. Concrete,
   present-day consequence, independent of any mingw `sig_action`
   work: `kill -TERM`, `trap ... INT` (by name), and `kill -l` all
   silently fail to resolve anything on that platform today. Reads as
   an oversight -- the table predates `sig.h`'s Windows number defines
   and was never updated to match.
2. **Two independent name-to-number resolvers.** `sig_byname()`
   strips a `"SIG"` prefix itself and returns `-1` on no match;
   `sig_number()` doesn't strip the prefix, uses libc `strcasecmp`,
   and returns `0` on no match -- which collides with `0` also being
   the real return for the `"EXIT"` pseudo-signal, forcing its one
   caller (`builtin_kill.c:24-31`) to carry an explicit disambiguation
   comment. Each resolver has exactly one caller. A third,
   `trap_byname()` (`builtin_trap.c:48`), layers `RETURN`/`DEBUG`/
   `EXIT` handling on top of `sig_byname()` specifically -- so two
   builtins that do conceptually the same job (`kill`, `trap`) go
   through two different underlying resolvers.
3. **Missing `return` on two functions under their excluded
   platform.** `sig_catch()` and `sig_push()` are both declared `int`,
   both `#if !(WINDOWS_NATIVE-ish guard)`-gated, and both fall off the
   end with no `return` statement when the guard excludes the
   platform -- UB, latent today only because no current caller checks
   the return value on that path. Fix before any mingw enablement work
   touches these files, not after.
4. **Dead/commented-out code, four spots:** `sig_block.c:11`'s
   `sigemptyset()` whose result is immediately discarded by the very
   next line's fetch-`sigprocmask()`; `sig_unblock.c:15-23`'s `#if 0`
   abandoned alternate implementation; `exec_program.c:80`'s
   `// sig_block(SIGINT);` sitting next to a live `sig_block(SIGCHLD)`;
   `sh_init.c:90-92`'s commented-out
   `signal(SIGTTOU/SIGTTIN, SIG_IGN)` pair, unexplained. None load-
   bearing; straightforward deletions.
5. **Inconsistent `#if !WINDOWS_NATIVE` re-guarding of already-
   self-guarded no-ops.** `sig_block`/`sig_unblock`/`sig_blocknone`
   are unconditionally safe no-ops on `WINDOWS_NATIVE` internally, yet
   roughly half their call sites in `job_fork.c`/`exec_program.c`
   wrap them in the guard again anyway. `exec_program.c` shows both
   styles in the same file: line 220's `sig_blocknone();` has no
   guard, line 251's does, ~30 lines apart, same function family. Not
   a correctness bug -- makes the guards that *do* matter (the ones
   wrapping real POSIX-only calls like `setpgid`/`tcsetpgrp` in the
   same blocks) harder to spot by eye.
6. **`kill_list()`'s hardcoded `31`** (`builtin_kill.c:70-85`)
   duplicating `sig_table.c`'s own upper bound instead of walking the
   table to its null terminator.
7. **`term_init.c`/`term_restore.c`'s raw `signal()` calls** bypass
   the whole `sig_action` apparatus -- no mask semantics, no stack,
   just `SIG_IGN`/`SIG_DFL`/one-shot handlers for
   `SIGTTIN`/`SIGTTOU`/`SIGWINCH`. Possibly a legitimate "this doesn't
   need the fuller machinery" case rather than an oversight, but
   there's no comment saying so, and nothing stops the next signal-
   related call site from picking this style instead of `sig_catch`
   for no principled reason.

## Strategy

**Phase 1 -- done.** Mechanical fixes, no design decision required:

- Populate `sig_table.c` for `WINDOWS_NATIVE` to match `lib/sig.h`'s
  own signal-number defines (items 1 above). Fixes `kill -l`/name-
  based `trap`/`kill` resolution on that platform independent of the
  `sig_action` gap itself.
- Fix `sig_stack.c`'s `NSIG` mismatch: it bounds-checks against the
  *host's* `NSIG` (23 on mingw), but `lib/sig.h` defines Windows
  signal numbers up to 31 (`SIGSYS`). Size the stack/bounds check off
  `lib/sig.h`'s own range on `WINDOWS_NATIVE`, not the host's `NSIG`.
  Directly relevant to the `mingw-missing-sig-action` work: fixing
  `sig_action` alone would still leave `SIGURG` through `SIGSYS`
  silently rejected by `sig_pusha`/`sig_pop`'s bounds check.
- Merge `sig_byname()`/`sig_number()` into one resolver (uniform
  `-1`-on-no-match, `"SIG"`-prefix stripped once); update
  `builtin_kill.c` and `trap_byname()`'s underlying call to match. Item
  2.
- Add explicit `return` statements to `sig_catch()`/`sig_push()`'s
  excluded-platform branches. Item 3.
- Delete the four dead-code spots. Item 4.
- Drop the redundant `#if !WINDOWS_NATIVE` guards around
  `sig_block`/`sig_unblock`/`sig_blocknone` call sites -- the callee
  already no-ops safely; the guard adds nothing and hides the calls
  that do matter. Item 5.
- Walk `sig_table.c` to its terminator in `kill_list()` instead of the
  hardcoded `31`. Item 6.

None of this changed behavior on any platform where the subsystem
already worked; it closed platform-specific gaps that contradicted
each other (`sig.h`'s numbers vs. `sig_table.c`'s empty table vs.
`sig_stack.c`'s host-`NSIG` bound). Verified: `sig_byname()` resolves
`kill -TERM`/`trap ... INT`/`kill -l` correctly on glibc (unchanged
behavior); `SHISH_NSIG` preprocesses to `32` under `cfg-mingw64`
(previously the host's `NSIG`, `23`); the mingw build's undefined-
reference list is unchanged at exactly `sig_action`, `kill`/`killpg`,
`tcsetpgrp` (`sig_number`/`sig_table.c`'s old gap no longer appear,
confirming the merge and the Windows table didn't regress anything);
`tests/*.sh`/`tests/fixed.sh` on glibc unchanged (423 passed, same 5
pre-existing failures as `main`). Landed as `fixes/204`.

**Phase 2 -- done.** The one real design decision, not mechanical:
whether `term_init.c`/`term_restore.c`'s raw `signal()` usage is a
deliberate lighter-weight tier or should be folded into the
`sig_action` path. Decided to keep it as a **documented** second tier
-- these are one-shot ignore/reset cases with no mask/restart
semantics to express, and forcing them through the full
`SA_MASKALL`/stack apparatus would add machinery without adding
correctness. The rule is now written down at the top of `lib/sig.h`:
`sig_catch`/`sig_push` for real traps with mask semantics; plain
`signal()` only for terminal-driver ignore/reset, never for anything
`trap` can observe -- so the next call site has something to follow
instead of picking a style ad hoc. No behavior change; verified via a
clean rebuild.

**Relationship to the mingw `sig_action` work
(`mingw-porting.md` section 3):**

Do Phase 1's `sig_table.c` and `sig_stack.c` fixes *before or
alongside* writing a mingw `sig_action()` implementation, not after.
Patching `sig_action` alone, against the current tree as-is, would
produce a mingw build where traps install successfully but
`trap - INT`/`kill -TERM` by name still silently fail to resolve
(item 1) and any trap on `SIGURG` through `SIGSYS` fails an artificial
bounds check that has nothing to do with `sig_action` itself (the
`NSIG` fix above). Both are small, and both are strictly upstream of
"does `trap` actually work end to end on mingw" -- fixing them first
means the eventual `sig_action` mingw port only has to solve the one
problem it's actually about (no real `sigaction(2)`/mask API to
translate onto), not rediscover these two once it starts being
exercised for real.

**Phase 3 -- resolved: no mingw `sig_action` shim.** After comparing
against the equivalent module in the sibling `c-utils` project,
decided *not* to build the `signal()`-based shim `mingw-porting.md`
section 3 originally proposed. `sig_action()` now compiles
unconditionally and returns `-1` honestly on `WINDOWS_NATIVE`
(documented at its declaration in `lib/sig.h`), matching `c-utils`'s
own stance: no real signal disposition on Windows, rather than a
lossy partial emulation of one. `sig_push()`/`sig_catch()` -- which
previously short-circuited to `return 0` on that platform, silently
claiming success while doing nothing -- now call through and let that
`-1` propagate for real. Phase 1's `sig_table.c`/`sig_stack.c` fixes
are unaffected: name/number lookup (`sig_name`, `sig_byname`,
`kill -l`) still works on `WINDOWS_NATIVE` independent of this: it's
disposition changes specifically that never take effect there. Also
fixed in the same change: `sig_catch.c`'s guard wrongly excluded MSYS
too (MSYS has a real `sigaction`, it isn't `WINDOWS_NATIVE`), so
msys64 was silently getting the same fake-success no-op -- and its
`.sa_restorer` designated-initializer field doesn't even exist outside
glibc, so the file had never actually compiled on that target before.
Landed as `fixes/207`.

## Not attempted here

`job_fork()`'s two-`sigprocmask()`-round-trip `SIGCHLD` block/unblock
around every `fork()` is real, minimal-for-what-it-guards-against
work, not redundancy -- simplifying it would mean changing the
race-avoidance strategy itself (e.g. blocking `SIGCHLD` for the whole
shell's lifetime and reaping via a different mechanism), which is a
bigger redesign than this document's scope and not something to
attempt as part of the cleanup above.
