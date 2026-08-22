# mingw porting strategy

`cfg-mingw64` now *compiles* cleanly (fixed by `fixes/200` and
`fixes/201`) but fails to *link*: nine undefined references, from
`@undefined-ref.txt`, deduplicated to five call sites' worth of
functionality:

```
getppid                                          src/sh/sh_init.c:46
mmap_read, mmap_unmap,
buffer_munmap, buffer_mmapread                   history_init.c, buffer_prefetch.c, path_gethome.c
killpg, kill                                     builtin_jobs.c, builtin_kill.c
tcsetpgrp                                        job_foreground.c
sig_action                                       lib/sig/sig_stack.c
```

This file is the plan for closing that list, ranked by leverage and
honest about which items are not fully portable at all. Each numbered
item below is meant to become its own `BUGS` entry (or, once fixed,
its own `fixes/NNN`) rather than one giant patch.

---

## 1. `mmap_read` / `mmap_unmap` / `buffer_munmap` / `buffer_mmapread`

**Not a porting task -- a build-system bug.** Every one of these
already has a working Windows implementation:

- `lib/mmap/mmap_read.c` -- `#ifdef _WIN32`, `CreateFileMapping` +
  `MapViewOfFile`
- `lib/mmap/mmap_read_fd.c`, `lib/mmap/mmap_unmap.c`,
  `lib/buffer/buffer_munmap.c` -- `#if WINDOWS_NATIVE`, `UnmapViewOfFile`
- `lib/buffer/buffer_mmapread.c` -- portable, just calls `mmap_read()`

`cmake/libowfat.cmake:8-10` strips all of `lib/*/*.c` matching
`mmap|munmap` whenever `HAVE_MMAP_SUPPORT` is false:

```cmake
if(NOT HAVE_MMAP_SUPPORT)
   list(FILTER LIBOWFAT_SOURCES EXCLUDE REGEX "lib.*([^d]mmap|munmap)")
endif(NOT HAVE_MMAP_SUPPORT)
```

`HAVE_MMAP_SUPPORT` (`cmake/Checks.cmake:302-308`) is a POSIX-only
probe (`<sys/mman.h>` + `mmap()` + `munmap()` found), so it's false on
mingw and the filter removes the very files whose `#if WINDOWS_NATIVE`
branch was written for this target -- 7 files gone:
`mmap_read.c`, `mmap_read_fd.c`, `mmap_unmap.c`, `mmap_filename.c`
(`lib/stralloc/`), `buffer_mmapread.c`, `buffer_mmapread_fd.c`,
`buffer_munmap.c`.

**Fix:** stop filtering them out on a Windows target, the same way
`cmake/Checks.cmake:102-105` already special-cases
`WIN32 OR WIN64 OR MINGW OR WINDOWS` for socket-library detection:

```cmake
if(NOT HAVE_MMAP_SUPPORT AND NOT (WIN32 OR WIN64 OR MINGW OR WINDOWS))
   list(FILTER LIBOWFAT_SOURCES EXCLUDE REGEX "lib.*([^d]mmap|munmap)")
endif()
```

No C changes needed. `path_gethome.c` reads `/etc/passwd` via this
path, which doesn't exist on Windows -- once compiled, it fails open
and returns `NULL`, which `src/sh/sh_gethome.c:16` already treats as a
safe fallback (`$HOME` is checked first anyway).

**Tier: trivial. Do this first** -- it clears 4 of 9 symbols for a
one-line CMake change, no runtime risk.

---

## 2. `getppid`

`src/sh/sh_init.c:46`: `var_setvint("PPID", getppid(), 0)`, unconditional
-- unlike `getuid()` two lines above it, which already has a
`#if WINDOWS_NATIVE` guard (`sh_init.c:35-38`).

No CRT/mingw equivalent exists. Two options:

- `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS)` +
  `Process32First`/`Process32Next`, scanning for
  `th32ProcessID == GetCurrentProcessId()` and reading
  `th32ParentProcessID`. Public, documented API.
- `NtQueryInformationProcess(ProcessBasicInformation)`, looked up via
  `GetProcAddress` on `ntdll.dll` -- undocumented, but matches this
  codebase's existing style: `src/fork.c` already resolves
  `RtlCloneUserProcess` the same way for its `WINDOWS_NATIVE` fork
  shim.

**Recommendation:** the Toolhelp32 route. `$PPID` is read-only shell
state nothing else depends on for correctness, so prefer the
documented API over another undocumented-ntdll dependency; fall back
to `var_setvint("PPID", 0, 0)` if the snapshot lookup fails.

**Tier: trivial.** ~15-20 lines, isolated, cosmetic blast radius.

---

## 3. `sig_action` and the rest of `lib/sig/`

`lib/sig/sig_action.c`'s entire body is `#ifdef SA_RESTART`. mingw's
`<signal.h>` defines none of `SA_RESTART`, `struct sigaction`,
`sigaction()`, `sigemptyset()`/`sigfillset()`/`sigismember()` -- so the
file compiles to nothing, and `sig_action` is genuinely absent, not
merely filtered out like case 1. (`/usr/x86_64-w64-mingw32/include/signal.h`
defines exactly 7 signals: `SIGINT SIGILL SIGABRT_COMPAT SIGFPE
SIGSEGV SIGTERM SIGBREAK SIGABRT` -- no mask/action API at all.)

This is not a leaf call: `sig_action` underlies `sig_stack.c`
(push/pop), `sig_push.c`, `sig_catch.c` -- all of `lib/sig/` -- plus
direct `struct sigaction` use in `src/sh/sh_main.c` and
`src/builtin/builtin_trap.c`. It's shish's entire trap/signal-
disposition architecture (7 files), and there is no way to give it
real POSIX semantics on Windows: no masking, no `SA_RESTART`, no
reliable multi-signal delivery, and the target signal set is 7 names
instead of ~30.

**Two-tier plan, in order:**

1. A minimal `sig_action` shim mapped onto plain `signal()` for the
   ~7 signals mingw's headers define. Explicitly drop mask,
   `SA_RESTART`, and `SA_NOCLDSTOP` semantics -- document in the shim
   itself, and in `BUGS`, exactly what `trap` behavior degrades as a
   result (no signal blocking during a trap handler, no restart-vs-
   interrupt distinction, nothing outside the 7-signal set is
   trappable at all).
2. Longer-term, real Ctrl-C/Ctrl-Break handling should go through
   `SetConsoleCtrlHandler`, not a POSIX-signal pretense. That's a
   separate, larger redesign of the console-input path, not a shim,
   and is out of scope until (1) exists and the size of the remaining
   gap is clearer.

**Tier: hard, but foundational** -- nothing about traps or signal
disposition works right on mingw until some version of this exists,
even a lossy one. Do this before item 4, since `kill`'s signal-number
semantics inherit the same "which of shish's signals even exist here"
question.

---

## 4. `kill` / `killpg`

`builtin_jobs.c:73` (`job_resume()`) sends `SIGCONT` to a job's whole
process group via `killpg(j->pgrp, SIGCONT)`; `builtin_kill.c:47,51,155`
does the same for `kill %job`/`kill pid` from the `kill` builtin,
falling back to per-pid `kill()` when there's no process group.

No shish wrapper exists for either -- they're raw libc calls, and
there's no general Windows equivalent:

- `TerminateProcess(handle, code)` approximates `SIGKILL` only: it's
  ungraceful and handle-based (needs `OpenProcess` first), not
  pid-based.
- `GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pgid)` can approximate
  signaling a *console process group*, but only if the target was
  created with `CREATE_NEW_PROCESS_GROUP`, and only delivers a
  Ctrl-C/Ctrl-Break equivalent -- `SIGSTOP`, `SIGCONT`, `SIGHUP`, and
  everything else in POSIX's signal set has no Windows analog at all.

**Fix is a decision table, not a drop-in function.** Before writing
any shim, decide per signal number what `kill -N` actually does on
Windows:

| signal | Windows action |
|---|---|
| `SIGKILL`, `SIGTERM` | `OpenProcess` + `TerminateProcess` |
| `SIGINT` (to a console process group) | `GenerateConsoleCtrlEvent(CTRL_C_EVENT, ...)` |
| `SIGCONT`, `SIGSTOP`, `SIGTSTP`, `SIGHUP`, ... | no-op or error -- document as unsupported, do not fake success |

**Tier: hard, partial coverage only ever possible.** Implement the
table above as `kill()`/`killpg()` shims once item 3 settles which
signal numbers exist to dispatch on; document the loss in `BUGS`
rather than presenting job control as working when most of it isn't.

---

## 5. `tcsetpgrp` (and its untracked siblings)

`src/job/job_foreground.c:13`: `tcsetpgrp(term_input.fd, job->pgrp)`,
bracketed by `sig_block(SIGTTOU)`/`sig_unblock(SIGTTOU)` -- hands the
controlling terminal to a job's process group so it becomes the
foreground job.

**Not in the original undefined-ref list, but the same subsystem and
will surface the moment this is touched:** `tcgetpgrp`
(`src/job/job_init.c:43`) and `setpgid`
(`src/job/job_fork.c:126,133,187`, `src/job/job_wait.c:196`).

Windows' console model has **no controlling-terminal or foreground-
process-group concept at all** -- console input goes to whichever
process owns or has attached the console, full stop. There is nothing
to map `tcsetpgrp`/`tcgetpgrp`/`setpgid` onto.

**Fix: compile interactive job control out entirely under
`WINDOWS_NATIVE`**, not shim functions with no target concept to
implement. Every job becomes synchronous/foreground-only, which is
also the honest description of what a pty-less environment can
actually support. `job_foreground.c`, the `setpgid` call sites in
`job_fork.c`/`job_wait.c`, and `job_init.c:43`'s `tcgetpgrp` all need
this guard together, as one change -- they're one coherent feature,
not four independent bugs.

**Tier: not portable.** Scope this as "disable a feature on this
platform," write it into `doc/building.md`'s mingw section once done,
and don't attempt a partial shim.

---

## Priority order

1. **mmap/buffer_mmap family** (§1) -- CMake-only, zero risk, clears 4
   of 9 symbols immediately. Do this first.
2. **`getppid`** (§2) -- trivial, isolated, cosmetic.
3. **`sig_action`** (§3) -- hard, but everything downstream (traps,
   `kill`'s signal set) depends on some version of it existing first.
4. **`kill`/`killpg`** (§4) -- hard, partial coverage only; needs §3
   settled first.
5. **`tcsetpgrp`/`tcgetpgrp`/`setpgid`** (§5) -- not portable; scope as
   disabling interactive job control under `WINDOWS_NATIVE`, last,
   since it's the most disruptive change and the least deferrable
   once decided.

Each item should land as its own `BUGS` entry before it's fixed and
its own `fixes/NNN` patch afterward, per this repo's normal workflow
(see `CLAUDE.md`) -- this file tracks the plan, not the bug reports.
