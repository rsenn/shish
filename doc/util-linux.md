# util-linux programs that are not builtins yet

Prioritized by estimated size, smallest first. The LOC column is an estimate of the work, not a
measurement. The list is `util-linux.unimplemented` (80 names). Almost all of it is **Linux-only** and
much of it needs root or a kernel interface (ioctl, sysfs, a syscall without a libc wrapper), so every
entry stays behind its own `BUILTIN_<NAME>` switch, **off by default**, and compiles only where the
needed headers exist.

**How to read it:**
- **LOC:** approximate lines for a shish-style builtin built on `lib/` primitives.
- **POSIX:** whether POSIX.1-2024 specifies the utility (`yes (XSI)` for the X/Open extension); only
  `more`, `mesg`, `ipcs` and `ipcrm` are in it. `getopt` is *not* POSIX: the `getopts` builtin is.
- **filter:** whether it can reasonably sit on the existing filter framework (stdin to stdout).
- **goes in:** the source file the utility belongs in (several utilities share one file, see below). They
  live in `src/builtin/extra/` (another package's programs) except where a name says `core/`; each gets one
  line in `src/builtin/builtins.map` (planned, see `TODO.md`), one row in `builtin_table.c`, a `help_<name>`
  string, and its code behind `#if BUILTIN_<NAME>`.
- **category:** fs, text, system, tty, proc, user, ipc, misc.

| # | name | POSIX | filter | category | ~LOC | goes in | what it does | note |
|---|---|---|---|---|---|---|---|---|
| 1 | i386 | no | no | proc | 5 | `extra/builtin_setarch.c` | run a command with the 32-bit x86 personality | alias of `setarch i386` |
| 2 | linux32 | no | no | proc | 5 | `extra/builtin_setarch.c` | run a command with the 32-bit personality | alias of `setarch linux32` |
| 3 | linux64 | no | no | proc | 5 | `extra/builtin_setarch.c` | run a command with the 64-bit personality | alias of `setarch linux64` |
| 4 | x86_64 | no | no | proc | 5 | `extra/builtin_setarch.c` | run a command with the x86-64 personality | alias of `setarch x86_64` |
| 5 | lastb | no | no | user | 5 | `core/builtin_utmp.c` | list the bad login attempts | `last` reading `btmp` |
| 6 | ctrlaltdel | no | no | system | 20 | `extra/builtin_hotplug.c` | set what Ctrl-Alt-Del does | `reboot(2)` with `RB_ENABLE_CAD`/`RB_DISABLE_CAD`; root only |
| 7 | pivot_root | no | no | system | 25 | `extra/builtin_ns.c` | swap the root file system | `pivot_root` syscall (no libc wrapper) |
| 8 | mcookie | no | generator | misc | 30 | `extra/builtin_mcookie.c` | print a random 128-bit hex cookie | read `/dev/urandom` |
| 9 | setsid | no | no | proc | 30 | `extra/builtin_sched.c` | run a command in a new session | `setsid(2)` + exec, `-c -w -f` |
| 10 | fsfreeze | no | no | fs | 30 | `extra/builtin_blkdev.c` | freeze or thaw a mounted file system | `FIFREEZE`/`FITHAW` ioctl |
| 11 | isosize | no | no | fs | 30 | `extra/builtin_blkdev.c` | print the size of an ISO-9660 file system | read the volume descriptor |
| 12 | addpart | no | no | fs | 30 | `extra/builtin_blkdev.c` | tell the kernel about a partition | `BLKPG_ADD_PARTITION` ioctl |
| 13 | delpart | no | no | fs | 30 | `extra/builtin_blkdev.c` | tell the kernel to forget a partition | `BLKPG_DEL_PARTITION` ioctl |
| 14 | resizepart | no | no | fs | 35 | `extra/builtin_blkdev.c` | tell the kernel a partition changed size | `BLKPG_RESIZE_PARTITION` ioctl |
| 15 | mesg | yes | no | tty | 35 | `core/builtin_stty.c` | allow or refuse `write` messages to the terminal | `fchmod` of the tty `g+w` |
| 16 | rev | no | yes | text | 35 | `extra/builtin_rev.c` | reverse the characters of every line | UTF-8 aware when `WITH_UTF8` |
| 17 | choom | no | no | proc | 40 | `extra/builtin_sched.c` | show or set the OOM-killer score adjustment | `/proc/PID/oom_score_adj` |
| 18 | blkdiscard | no | no | fs | 40 | `extra/builtin_blkdev.c` | discard sectors of a device | `BLKDISCARD` ioctl, `-o -l -s` |
| 19 | findfs | no | no | fs | 40 | `extra/builtin_blkid.c` | find a file system by label or UUID | uses the `blkid` probe |
| 20 | mountpoint | no | no | fs | 50 | `extra/builtin_mountpoint.c` | tell whether a directory is a mount point | compare `st_dev`, or `/proc/self/mountinfo`; `-q -d -x` |
| 21 | fallocate | no | no | fs | 50 | `extra/builtin_fallocate.c` | preallocate or punch holes in a file | `fallocate(2)`, `-l -o -n -p -d` |
| 22 | chcpu | no | no | system | 60 | `extra/builtin_hotplug.c` | enable or disable CPUs | sysfs writes, `-e -d -c -g -p` |
| 23 | ionice | no | no | proc | 60 | `extra/builtin_sched.c` | get or set the I/O scheduling class and priority | `ioprio_set` via `syscall()` |
| 24 | rename.ul | no | no | fs | 60 | `extra/builtin_rename.c` | rename files by replacing a substring | `rename.ul [-v -n -a] from to file...` |
| 25 | fstrim | no | no | fs | 60 | `extra/builtin_blkdev.c` | discard unused blocks of a mounted file system | `FITRIM` ioctl, `-a` reads mountinfo |
| 26 | uclampset | no | no | proc | 60 | `extra/builtin_sched.c` | set utilization clamps of a task | `sched_setattr` via `syscall()` |
| 27 | ipcrm | yes (XSI) | no | ipc | 70 | `extra/builtin_ipc.c` | remove System V IPC objects | `msgctl`/`semctl`/`shmctl` `IPC_RMID` |
| 28 | setarch | no | no | proc | 70 | `extra/builtin_setarch.c` | run a command with a different architecture personality | `personality(2)` + exec, `-R` (no ASLR) |
| 29 | flock | no | no | fs | 70 | `extra/builtin_flock.c` | lock a file and run a command | `flock(2)`, `-s -x -n -w -u -c` |
| 30 | chrt | no | no | proc | 80 | `extra/builtin_sched.c` | show or set the real-time scheduling attributes | `sched_setscheduler` |
| 31 | taskset | no | no | proc | 80 | `extra/builtin_sched.c` | show or set the CPU affinity of a process | `sched_setaffinity`, CPU list parser |
| 32 | blockdev | no | no | fs | 80 | `extra/builtin_blkdev.c` | call block device ioctls from the command line | `--getsize64 --setro --flushbufs` ... |
| 33 | dmesg | no | no | system | 80 | `extra/builtin_dmesg.c` | print the kernel ring buffer | `klogctl` or `/dev/kmsg`, `-c -n -T` |
| 34 | chmem | no | no | system | 80 | `extra/builtin_hotplug.c` | set memory blocks online or offline | sysfs; shares the range parser with `chcpu` |
| 35 | ipcmk | no | no | ipc | 80 | `extra/builtin_ipc.c` | create System V IPC objects | `msgget`/`semget`/`shmget` |
| 36 | mkfs | no | no | fs | 80 | `extra/builtin_mkfs.c` | build a Linux file system | wrapper that execs `mkfs.TYPE` |
| 37 | ldattach | no | no | tty | 80 | `extra/builtin_ldattach.c` | attach a line discipline to a serial line | `TIOCSETD` ioctl, then wait |
| 38 | mkswap | no | no | fs | 100 | `extra/builtin_blkid.c` | set up a swap area | write the swap header with UUID and label |
| 39 | namei | no | no | fs | 100 | `extra/builtin_namei.c` | follow a path and print every component | `lstat` + `readlink` loop |
| 40 | readprofile | no | no | system | 100 | `extra/builtin_readprofile.c` | read kernel profiling information | `/proc/profile` plus the map file |
| 41 | wdctl | no | no | system | 100 | `extra/builtin_hotplug.c` | show watchdog status | watchdog ioctls |
| 42 | blkzone | no | no | fs | 100 | `extra/builtin_blkdev.c` | run zone commands on a zoned block device | zoned block ioctls |
| 43 | utmpdump | no | yes | user | 100 | `core/builtin_utmp.c` | dump utmp/wtmp as text or load it back | shares the utmp reader with `who` |
| 44 | last | no | no | user | 100 | `core/builtin_utmp.c` | list the last logged-in users | reads `wtmp`; shares the utmp reader |
| 45 | lslocks | no | no | system | 100 | `extra/builtin_lstab.c` | list local system locks | parse `/proc/locks` |
| 46 | swaplabel | no | no | fs | 100 | `extra/builtin_blkid.c` | show or change the label and UUID of a swap area | edit the swap header |
| 47 | switch_root | no | no | system | 120 | `extra/builtin_ns.c` | switch to another root file system and exec `init` | mount move, chroot, exec; deletes the old root with the directory walker |
| 48 | whereis | no | no | fs | 120 | `extra/builtin_which.c` | locate binary, source and manual files for a command | shares the directory-list search loop with `which` |
| 49 | runuser | no | no | user | 120 | `extra/builtin_su.c` | run a command as another user and group (root only) | `setgroups`/`setgid`/`setuid` + exec, no password |
| 50 | lsmem | no | no | system | 120 | `extra/builtin_lstab.c` | list the ranges of available memory | sysfs memory blocks |
| 51 | lsns | no | no | proc | 120 | `extra/builtin_lstab.c` | list namespaces | walk `/proc/*/ns` |
| 52 | prlimit | no | no | proc | 120 | `builtin_ulimit.c` | show or set the resource limits of a running process | `prlimit(2)`; shares the `RLIMIT_*` table of `ulimit` |
| 53 | getopt | no | no | text | 150 | `extra/builtin_getopt.c` | parse options for shell scripts (util-linux `getopt`) | GNU-style quoted output; POSIX has the `getopts` builtin instead |
| 54 | hardlink | no | no | fs | 150 | `extra/builtin_hardlink.c` | replace identical files by hard links | uses the directory walker; compare size, then hash or bytes |
| 55 | rtcwake | no | no | system | 150 | `extra/builtin_hotplug.c` | enter a sleep state until a wake-up time | `/sys/class/rtc` and `/sys/power/state` |
| 56 | unshare | no | no | proc | 150 | `extra/builtin_ns.c` | run a command in new namespaces | `unshare(2)`/`clone` flags + exec |
| 57 | nsenter | no | no | proc | 150 | `extra/builtin_ns.c` | run a command in the namespaces of another process | `setns(2)` + exec |
| 58 | ipcs | yes (XSI) | no | ipc | 150 | `extra/builtin_ipc.c` | show System V IPC status | `msgctl`/`semctl`/`shmctl` `*_STAT`; the table printer |
| 59 | wipefs | no | no | fs | 150 | `extra/builtin_blkid.c` | wipe file system signatures from a device | needs the signature table of `blkid` |
| 60 | lsipc | no | no | ipc | 180 | `extra/builtin_lstab.c` | show information about IPC facilities | `ipcs` data in the common column table |
| 61 | lscpu | no | no | system | 200 | `extra/builtin_lstab.c` | show CPU architecture information | `/proc/cpuinfo` + sysfs |
| 62 | lslogins | no | no | user | 200 | `extra/builtin_lstab.c` | show information about known users | passwd, shadow, utmp and lastlog |
| 63 | setterm | no | no | tty | 200 | `extra/builtin_setterm.c` | set terminal attributes | escape sequences and `ioctl` |
| 64 | more | yes | yes (needs a tty) | tty | 250 | `extra/builtin_more.c` | page through text one screen at a time | scrolling and `/` search with termios raw mode |
| 65 | lsblk | no | no | fs | 250 | `extra/builtin_lstab.c` | list block devices as a tree | sysfs; `-o` columns |
| 66 | findmnt | no | no | fs | 250 | `extra/builtin_lstab.c` | find and list mounted file systems as a tree | `/proc/self/mountinfo` |
| 67 | zramctl | no | no | system | 250 | `extra/builtin_hotplug.c` | set up and control zram devices | sysfs under `/sys/block/zram*` |
| 68 | partx | no | no | fs | 250 | `extra/builtin_blkdev.c` | tell the kernel about partitions | reads the MBR and GPT tables, then `BLKPG` |
| 69 | setpriv | no | no | proc | 250 | `extra/builtin_sched.c` | run a command with different privileges | capabilities, securebits, `no_new_privs`, ids |
| 70 | blkid | no | no | fs | 300 | `extra/builtin_blkid.c` | print block device attributes (UUID, LABEL, TYPE) | superblock probing for many file system types |
| 71 | su | no | no | user | n/a | - | become another user | needs password checking (PAM or shadow); not worth a builtin |
| 72 | sulogin | no | no | user | n/a | - | single-user login | needs password checking; a system program |
| 73 | getty | no | no | tty | n/a | - | open a terminal and start a login | a login service, not a shell command |
| 74 | agetty | no | no | tty | n/a | - | alternative Linux getty | a login service, not a shell command |
| 75 | fsck | no | no | fs | n/a | - | check and repair a file system | wrapper that execs `fsck.TYPE`; useless without the checkers |
| 76 | fsck.minix | no | no | fs | n/a | - | check a Minix file system | format-specific, large; not worth a builtin |
| 77 | fsck.cramfs | no | no | fs | n/a | - | check a cramfs file system | format-specific; not worth a builtin |
| 78 | mkfs.bfs | no | no | fs | n/a | - | build a SCO bfs file system | format-specific; not worth a builtin |
| 79 | mkfs.minix | no | no | fs | n/a | - | build a Minix file system | format-specific; not worth a builtin |
| 80 | mkfs.cramfs | no | no | fs | n/a | - | build a compressed ROM file system | format-specific, needs zlib; not worth a builtin |

(The last block is not worth building as builtins; see "Skipped".)

## Utilities that share one source file

"goes in" names the source file. A file with several utilities holds all of them, each behind its own
`#if BUILTIN_<NAME>` switch. Files that already exist, or are planned in `doc/coreutils.md`, are marked.

| file | utilities | what they share | ~LOC saved |
|---|---|---|---|
| `builtin_setarch.c` | setarch, i386, linux32, linux64, x86_64 | one `personality(2)` wrapper; the four names are aliases that preselect the personality | 25 |
| `builtin_sched.c` | chrt, taskset, ionice, uclampset, choom, setsid, setpriv | the "apply an attribute to this process or a pid, then exec" helper that `timeout` and the planned `nice`/`nohup` also need, plus `PID`/command argument parsing | 150 |
| `builtin_ulimit.c` (existing) | prlimit | the `RLIMIT_*` resource table of `ulimit`; `prlimit` adds a pid and `prlimit(2)` | 40 |
| `builtin_ns.c` | nsenter, unshare, pivot_root, switch_root | namespace flag table (`CLONE_NEW*`) and the "enter, then exec" tail; `switch_root` also needs the directory walker | 60 |
| `builtin_ipc.c` | ipcs, ipcrm, ipcmk | the `msg`/`sem`/`shm` triple of `*get`, `*ctl` calls behind one set of option letters (`-q -s -m`) | 90 |
| `builtin_utmp.c` (planned) | last, lastb, utmpdump | the utmp/wtmp record reader and time formatter of `who`, `users` and `pinky` | 80 |
| `builtin_blkdev.c` | blockdev, blkdiscard, blkzone, fsfreeze, fstrim, isosize, addpart, delpart, resizepart, partx | open a device, one `ioctl`, print the result; the partition table reader of `partx` also feeds `addpart` | 150 |
| `builtin_blkid.c` | blkid, findfs, wipefs, swaplabel, mkswap | one superblock signature table (magic, offset, label and UUID positions) | 200 |
| `builtin_hotplug.c` | chcpu, chmem, rtcwake, wdctl, zramctl, ctrlaltdel | write a value into a sysfs/proc file and read it back; `chcpu` and `chmem` share the range parser (`0-3,5`) | 80 |
| `builtin_su.c` | runuser (su, sulogin skipped) | `setgroups`/`setgid`/`setuid` + exec | 30 |
| `builtin_stty.c` (planned) | mesg | the terminal-device lookup of `tty`/`stty` | 15 |
| `builtin_which.c` (existing) | whereis | the search loop over a list of directories | 40 |

**Listing tools** (`lscpu`, `lsmem`, `lsblk`, `lslocks`, `lslogins`, `lsns`, `lsipc`, `findmnt`) all print a table with
`-o COLUMNS`, `-n` (no heading), `-r` (raw) and `-J` (JSON). They go in `builtin_lstab.c` and share one
column-table printer, planned as `lib/coltab.h` + `lib/coltab/coltab_*.c` (a column list with a name, a width, a
right-align flag and a per-row string callback); written once it is about 100 lines, and it removes
about 40 per tool.

**Kept apart:** `flock`, `mountpoint`, `fallocate`, `rev`, `mcookie`, `getopt`, `namei`, `rename.ul`, `dmesg`,
`hardlink`, `more`, `setterm`, `ldattach`, `readprofile` have nothing to share with another utility.

**Directory walker.** `hardlink` and `switch_root` (deleting the old root) need the walker planned in `TODO.md`
("Part C", `lib/walk`); `findmnt`, `lsblk` and `lsns` only read `/proc` and `/sys` and need no walker.

## Skipped

`su`, `sulogin` (password checking needs PAM or shadow), `getty`, `agetty` (login services), `fsck`, `fsck.minix`,
`fsck.cramfs`, `mkfs.bfs`, `mkfs.minix`, `mkfs.cramfs` (format-specific code; a builtin adds nothing to the
standalone program, and the `fsck`/`mkfs` wrappers are useless without them). `runuser` stays because it needs no
password. Also `mkfs` itself is a wrapper that only execs `mkfs.TYPE`, so it is worth it only if one of the
`mkfs.*` helpers exists on the system.

## What to do in order

1. **Batch 1, scripting staples, about 300 lines:** `rev mcookie mesg setsid mountpoint fallocate flock namei isosize`.
   No kernel-interface surprises, testable as a normal user.
2. **Batch 2, process wrappers:** `chrt taskset ionice choom uclampset setarch` (and `i386 linux32 linux64 x86_64`), then
   `prlimit` on top of `ulimit`. They share the exec helper with `nice`/`nohup`, so do them after those.
3. **Batch 3, namespaces:** `unshare nsenter pivot_root switch_root`; test with `unshare -Ur`, which needs no root.
4. **Batch 4, tables and information:** `lib/coltab` first, then `lscpu lsmem lslocks lsns lsblk findmnt`, then
   `last lastb utmpdump` once `builtin_utmp.c` exists.
5. **Batch 5, devices and IPC:** `ipcs ipcrm ipcmk lsipc`, `blockdev fsfreeze fstrim blkdiscard`, then the
   `blkid` family.
6. **Last, interactive and large:** `more` (needs raw termios, shares the work with `stty`), `getopt`, `setterm`,
   `setpriv`, `dmesg`, `rtcwake`, `zramctl`, `partx`, `hardlink`.

## Open questions

- **Is a Linux-only builtin wanted at all?** If shish is meant to be a busybox-like single binary, batches 1-3 are the
  useful part; if not, `coreutils.md` is the better use of the time. The plan assumes the former.
- **Portability gates:** every file needs `configure` checks (`HAVE_SCHED_SETAFFINITY`, `HAVE_SETNS`, ...), some syscalls
  have no libc wrapper (`ioprio_set`, `sched_setattr`, `pivot_root`) and must go through `syscall()` with the
  `SYS_*` numbers, and dietlibc and musl differ. The Windows and wasm builds must not see these files at all.
- **Testing:** most of them are root-only. The tests would run as a normal user in a user namespace (`unshare -Ur`) and
  skip themselves otherwise, like `tests/builtin-dirstack.sh` skips when the builtins are missing.
