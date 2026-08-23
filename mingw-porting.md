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

`HAVE_MMAP_SUPPORT` (`cmake/Checks.cmake:301-308`, before this fix) was
a POSIX-only probe (`<sys/mman.h>` + `mmap()` + `munmap()` found), so
it was false on mingw. That did two things at once: it removed the
very files whose `#if WINDOWS_NATIVE` branch was written for this
target from `cmake/libowfat.cmake:8-10`'s `list(FILTER ...)` -- 7 files
gone (`mmap_read.c`, `mmap_read_fd.c`, `mmap_unmap.c`,
`mmap_filename.c` in `lib/stralloc/`, `buffer_mmapread.c`,
`buffer_mmapread_fd.c`, `buffer_munmap.c`) -- *and* it force-disabled
`USE_MMAP`/`HAVE_MMAP` (`cmake/Checks.cmake:310-318`), silently routing
every mmap consumer (e.g. `src/fd/fd_mmap.c`) through the non-mmap
fallback even on a rebuild where the filter above was fixed.

**Fixed (`fixes/202`):** the real issue was the probe asking only
about POSIX mmap. `HAVE_MMAP_SUPPORT` now also becomes true when
targeting `WIN32 OR WIN64 OR MINGW OR WINDOWS` (the same platform test
`cmake/Checks.cmake:102-105` already uses for socket-library
detection), since Windows' `CreateFileMapping`-based code in
`lib/mmap/`/`lib/buffer/` *is* this platform's mmap support, not an
absence of it. That one change fixes both symptoms: the filter no
longer strips the 7 files (its condition was always just
`NOT HAVE_MMAP_SUPPORT`), and `USE_MMAP`/`HAVE_MMAP` now stay on, so
`fd_mmap.c` actually takes the mmap path on mingw instead of silently
falling back. No `libowfat.cmake` change needed in the end, and no C
changes. `path_gethome.c` reads `/etc/passwd` via this path, which
doesn't exist on Windows -- it fails open and returns `NULL`, which
`src/sh/sh_gethome.c:16` already treats as a safe fallback (`$HOME` is
checked first anyway).

**Tier: trivial. Done** -- cleared 4 of 9 symbols and turned on the
mmap I/O path on mingw for real, for a doc-comment-sized `cmake/Checks.cmake`
change, no runtime risk (verified: glibc `tests/*.sh`/`tests/fixed.sh`
unchanged, mingw compiles clean, `config.h` shows `HAVE_MMAP 1` with no
warning).

---

## 2. `getppid`

`src/sh/sh_init.c:46` (before this fix): `var_setvint("PPID",
getppid(), 0)`, unconditional -- unlike `getuid()` two lines above it,
which already has a `#if WINDOWS_NATIVE` guard (`sh_init.c:35-38`).

No CRT/mingw equivalent exists. Two options were considered:

- `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS)` +
  `Process32First`/`Process32Next`, scanning for
  `th32ProcessID == GetCurrentProcessId()` and reading
  `th32ParentProcessID`. Public, documented API.
- `NtQueryInformationProcess(ProcessBasicInformation)`, looked up via
  `GetProcAddress` on `ntdll.dll` -- undocumented, but matches this
  codebase's existing style: `src/fork.c` already resolves
  `RtlCloneUserProcess` the same way for its `WINDOWS_NATIVE` fork
  shim.

**Fixed (`fixes/203`):** took the Toolhelp32 route -- `$PPID` is
read-only shell state nothing else depends on for correctness, so the
documented API won over another undocumented-ntdll dependency. Landed
as two pieces:

- `getppid` added to `CMakeLists.txt`'s existing `check_functions(...)`
  call, so `HAVE_GETPPID` is now probed the same way `sigaction`,
  `signal`, etc. already are, instead of assumed.
- `lib/unix/getppid.c` provides a real `getppid()` (returning `pid_t`,
  falling back to 0 if the snapshot lookup fails), `#if WINDOWS_NATIVE`
  only -- named and shaped exactly like `lib/unix/readlink.c`'s own
  `WINDOWS_NATIVE`-only `readlink()`, so callers don't need to know
  which platform provided the symbol. `lib/unix.h` declares it under
  the same guard.

`sh_init.c` now just says
`#if defined(HAVE_GETPPID) || WINDOWS_NATIVE` /
`var_setvint("PPID", getppid(), 0)` -- one call site, no
platform-specific function name leaking into `src/`.

**Tier: trivial. Done.** ~35 lines total, isolated, cosmetic blast
radius. Verified: mingw links clean (`getppid` gone from the
undefined-reference list, leaving only `kill`/`killpg`, `tcsetpgrp`,
`sig_action`); glibc/dietlibc `$PPID` still reports the real parent
pid, `tests/*.sh`/`tests/fixed.sh` unchanged. Also surfaced, not
fixed: `sh_init.c:44`'s unconditional `getpid()` call triggers an
implicit-declaration *warning* (not a link failure) on mingw, since
`<unistd.h>` is only pulled in `#if !WINDOWS_NATIVE` -- logged as
`BUGS: mingw-getpid-implicit-declaration`.

---

## 3. `sig_action` and the rest of `lib/sig/` -- resolved

Was: `lib/sig/sig_action.c`'s entire body was `#ifdef SA_RESTART`, and
mingw's `<signal.h>` defines none of `SA_RESTART`, `struct sigaction`,
`sigaction()`, `sigemptyset()`/`sigfillset()`/`sigismember()` -- so the
file compiled to nothing, and `sig_action` was undefined at link time.
(`/usr/x86_64-w64-mingw32/include/signal.h` defines exactly 7 signals:
`SIGINT SIGILL SIGABRT_COMPAT SIGFPE SIGSEGV SIGTERM SIGBREAK SIGABRT`
-- no mask/action API at all.)

Decision, after comparing against the equivalent module in the
sibling `c-utils` project: don't build the `signal()`-based shim this
section used to propose. `sig_action()` now compiles unconditionally
and returns `-1` honestly on `WINDOWS_NATIVE` (`lib/sig.h`'s own
comment above its declaration) -- matching `c-utils/lib/sig`'s stance
of giving up on real signal disposition on Windows rather than
half-emulating it. `sig_push`/`sig_pop`/`sig_catch` all propagate that
`-1` through the same call chain instead of pretending success (they
previously short-circuited to `return 0` on this platform, which was
worse: silent no-op). `sig_name`/`sig_byname` (name/number lookup,
`kill -l`) are unaffected -- already fixed in `signal-refactor.md`
Phase 1 and independent of whether `sig_action` can actually install
anything. Landed as `fixes/207`.

Also fixed in the same change: `sig_catch.c` was additionally guarded
`#if !(defined(_WIN32) || defined(__MSYS__))`, wrongly treating MSYS
the same as `WINDOWS_NATIVE` even though MSYS has a real `sigaction`
(it's not `WINDOWS_NATIVE`, see `lib/windoze.h`) -- msys64 builds were
silently getting the same fake-success no-op as native Windows. Now
uses real `sig_action` like every other non-`WINDOWS_NATIVE` target.

---

## 4. `kill` / `killpg` -- resolved

Was: `builtin_jobs.c:73` (`job_resume()`) sends `SIGCONT` to a job's
whole process group via `killpg(j->pgrp, SIGCONT)`; `builtin_kill.c`
does the same for `kill %job`/`kill pid`, falling back to per-pid
`kill()` when there's no process group. Neither had a shish wrapper --
both were raw libc calls mingw doesn't provide, undefined at link
time.

Implemented per the decision table this section used to propose, in
`lib/unix/kill.c` and `lib/unix/killpg.c` (same
one-function-per-file/`WINDOWS_NATIVE`-gated convention as
`lib/unix/getppid.c`):

| signal | Windows action |
|---|---|
| `SIGKILL`, `SIGTERM` | `OpenProcess` + `TerminateProcess` |
| `SIGINT` | `GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid)` -- only works if `pid` is a real console process-group id (`CREATE_NEW_PROCESS_GROUP`); shish doesn't create children that way yet, so this currently just fails honestly for ordinary pids, same as the next row |
| `0` | existence check only (`OpenProcess` + close), no signal sent |
| everything else | `ENOSYS` -- no Windows analog, fails rather than faking success |

`killpg()` delegates straight to `kill()`: §5 (`setpgid`) is still
unimplemented, so `job->pgrp` is never a real Windows process group,
only ever the leader's own pid (`job_fork.c`) -- signaling just that
one process is the best available approximation until §5 exists.
Landed as `fixes/208`.

---

## 5. `tcsetpgrp` (and its untracked siblings) -- resolved

Was: `src/job/job_foreground.c:13`'s `tcsetpgrp(term_input.fd,
job->pgrp)`, `job_init.c:43`'s `tcgetpgrp`, and `setpgid`
(`job_fork.c:126,133,187`, `job_wait.c:196`) -- undefined at link time
on mingw. Windows' console model has **no controlling-terminal or
foreground-process-group concept at all**, so there was nothing to map
any of them onto.

`job_init.c`'s `tcgetpgrp` and `job_fork.c`/`job_wait.c`'s `setpgid`
were already `#if !WINDOWS_NATIVE`-gated from earlier, untracked work;
`job_foreground.c`'s `tcsetpgrp` gained the same guard as part of this
change. None of the three symbols are an actual link failure once all
three are gated. What
*was* still live on `WINDOWS_NATIVE`: `sh->opts.monitor` itself, set
unconditionally for any interactive session
(`sh_main.c`) and settable via `set -m`
(`builtin_set.c`) -- meaning job-control bookkeeping gated on
`sh->opts.monitor` (stop/resume announcements, `wait_pid_untraced`
selection, ...) still ran even though the `setpgid`/`tcsetpgrp`
primitives it depends on never actually execute there. Both now force
`monitor` to stay `0` under `WINDOWS_NATIVE`, completing the "compile
interactive job control out entirely" fix this section originally
called for: every job is synchronous/foreground-only on that platform,
consistently, not just at the syscall layer. Landed as `fixes/209`.

---

## Priority order

1. **mmap/buffer_mmap family** (§1) -- **done**, `fixes/202`. CMake-only,
   cleared 4 of 9 symbols and turned on the mmap I/O path on mingw for
   real.
2. **`getppid`** (§2) -- **done**, `fixes/203`. Trivial, isolated,
   cosmetic.
3. **`sig_action`** (§3) -- **done**, `fixes/207`. Resolved as "fail
   honestly, no shim" rather than implemented as originally planned;
   unblocks §4's `kill`/`killpg` on the "which signals exist" question.
4. **`kill`/`killpg`** (§4) -- **done**, `fixes/208`. Partial coverage
   only, as scoped: `SIGKILL`/`SIGTERM` via `TerminateProcess`,
   everything else fails honestly.
5. **`tcsetpgrp`/`tcgetpgrp`/`setpgid`** (§5) -- **done**, `fixes/209`.
   The three syscalls were already gated; the real fix was forcing
   `sh->opts.monitor` to stay off under `WINDOWS_NATIVE` so job-control
   bookkeeping doesn't run ahead of primitives that never execute
   there. `cfg-mingw64` now links clean end to end -- every symbol in
   this file's original undefined-reference list is resolved.

Each item should land as its own `BUGS` entry before it's fixed and
its own `fixes/NNN` patch afterward, per this repo's normal workflow
(see `CLAUDE.md`) -- this file tracks the plan, not the bug reports.
