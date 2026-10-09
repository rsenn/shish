# Coreutils programs that are not builtins yet

Prioritized by estimated size, smallest first. The LOC column is an estimate of the work, not a
measurement. The first line of `coreutils.unimplemented`, `[`, is already provided by the `test`
builtin and is left out.

**How to read it:**
- **LOC:** approximate lines for a shish-style builtin built on `lib/` primitives.
- **POSIX:** whether POSIX.1-2024 specifies the utility; `?` marks the ones that are uncertain.
- **filter:** whether it can reasonably sit on the existing filter framework (stdin to stdout, so it
  can chain in a pipeline).
- **goes in:** the source file the utility belongs in (several utilities share one file, see below). A new
  coreutils replacement lives in `src/builtin/core/`, gets one line in `src/builtin/builtins.map` (planned,
  see `TODO.md`; until then `cmake/Builtins.cmake`), one row in `src/builtin/builtin_table.c`, a
  `help_<name>` string, and its code behind `#if BUILTIN_<NAME>`.

| # | name | POSIX | filter | category | ~LOC | goes in | what it does | note |
|---|---|---|---|---|---|---|---|---|
| 1 | sync | no | no | fs | 10 | `core/builtin_sysinfo.c` | flush file system buffers to disk | one `sync()` call |
| 2 | arch | no | no | system | 15 | `core/builtin_uname.c` | print the machine hardware name | `uname -m` |
| 3 | hostid | no | no | system | 15 | `core/builtin_sysinfo.c` | print the numeric identifier of the host | `gethostid()`, `%08x` |
| 4 | yes | no | generator | misc | 25 | `core/builtin_yes.c` | repeat a string (default `y`) forever | repeat args until SIGPIPE |
| 5 | tty | yes | no | tty | 25 | `core/builtin_stty.c` | print the terminal name of stdin | `ttyname(0)`, `-s`, status 1 if not a tty |
| 6 | nproc | no | no | system | 25 | `core/builtin_sysinfo.c` | print the number of processing units | `sysconf(_SC_NPROCESSORS_ONLN)`, `--all`, `--ignore` |
| 7 | whoami | no | no | user | 25 | `core/builtin_id.c` | print the effective user name | `getpwuid(geteuid())` |
| 8 | logname | yes | no | user | 25 | `core/builtin_id.c` | print the login name of the user | `getlogin()`, fall back to `$LOGNAME` |
| 9 | printenv | no | no | proc | 35 | `core/builtin_env.c` | print environment variables | walk `environ` |
| 10 | md5sum.textutils | no | yes | hash | 5 | `extra/builtin_digest.c` | the legacy name of `md5sum` | alias of `md5sum`/`digest` |
| 11 | b2sum | no | yes | hash | 15 | `extra/builtin_digest.c` | print or check BLAKE2 checksums | one table row in `digest`, if its hash library has BLAKE2 |
| 12 | sum | no | yes | hash | 45 | `core/builtin_cksum.c` | print a checksum and block count | BSD and SysV checksums |
| 13 | tac | no | yes (buffers all input) | text | 50 | `core/builtin_tac.c` | print lines in reverse order | reverse lines |
| 14 | truncate | no | no | fs | 50 | `core/builtin_truncate.c` | shrink or extend a file to a size | `-s [+-/%]size`, `-c`, `-r` |
| 15 | mkfifo | yes | no | fs | 45 | `core/builtin_mknod.c` | create named pipes | `-m mode`, reuses `chmod_symbolic()` |
| 16 | nice | yes | no | proc | 55 | `core/builtin_nice.c` | run a command with a changed priority | `nice -n N cmd`, `setpriority`, then exec |
| 17 | nohup | yes | no | proc | 55 | `core/builtin_nice.c` | run a command immune to hangups | ignore SIGHUP, redirect to `nohup.out`, exec |
| 18 | groups | no | no | user | 50 | `core/builtin_id.c` | print the groups a user is in | `getgrouplist` |
| 19 | users | no | no | user | 40 | `core/builtin_utmp.c` | print the users logged in | read utmp |
| 20 | chroot | no | no | proc | 45 | `core/builtin_chroot.c` | run a command with another root directory | `chroot` plus exec, `--userspec` optional |
| 21 | seq | no? | generator | misc | 70 | `core/builtin_seq.c` | print a sequence of numbers | `-s`, `-w`, `-f`, floats via the printf number code |
| 22 | cksum | yes | yes | hash | 70 | `core/builtin_cksum.c` | print a CRC checksum and byte count | POSIX CRC (not zlib CRC), table driven |
| 23 | factor | no | yes (reads numbers) | misc | 75 | `core/builtin_factor.c` | print the prime factors of numbers | 64-bit trial division is enough |
| 24 | expand | yes | yes | text | 80 | `core/builtin_expand.c` | convert tabs to spaces | `-t list` |
| 25 | comm | yes | takes two files, so no | text | 80 | `core/builtin_join.c` | compare two sorted files line by line | `-1 -2 -3`, sorted input |
| 26 | mknod | no | no | fs | 60 | `core/builtin_mknod.c` | create special files | b/c/p/u nodes |
| 27 | pathchk | yes | no | fs | 90 | `core/builtin_pathchk.c` | check whether file names are valid and portable | `-p`, `-P`, `PATH_MAX`/`NAME_MAX` checks |
| 28 | unexpand | yes | yes | text | 90 | `core/builtin_expand.c` | convert spaces to tabs | `-a`, `-t`, `--first-only` |
| 29 | fold | yes | yes | text | 90 | `core/builtin_fold.c` | wrap lines to a width | `-b -s -w` (UTF-8 aware if `WITH_UTF8`) |
| 30 | base64 | no | yes | text | 100 | `core/builtin_basenc.c` | encode or decode base64 | encode and decode, `-w`, `-d`, `-i` |
| 31 | base32 | no | yes | text | 100 | `core/builtin_basenc.c` | encode or decode base32 | same shape as `base64`, shares most of the code |
| 32 | chown | yes | no | fs | 100 | `core/builtin_chown.c` | change file owner and group | `-R -h -H -L -P`, `user:group`; shares the tree walk with `chmod`; uses the directory walker (`lib/walk`, see TODO.md "PLAN") |
| 33 | chgrp | yes | no | fs | 90 | `core/builtin_chown.c` | change file group | same walker as `chown`; uses the directory walker (`lib/walk`, see TODO.md "PLAN") |
| 34 | shuf | no | yes (buffers) | text | 100 | `core/builtin_tac.c` | shuffle lines randomly | `-n -e -i -r`, needs a PRNG |
| 35 | tsort | yes | yes | text | 120 | `core/builtin_tsort.c` | topological sort of pairs | topological sort |
| 36 | shred | no | no | fs | 120 | `core/builtin_shred.c` | overwrite a file to hide its contents | overwrite passes |
| 37 | dir | no | no | fs | 10 | `core/builtin_ls.c` | list directory contents in columns | `ls -C -b`; needs `ls -C -b` first (open in `ls-missing-options`) |
| 38 | vdir | no | no | fs | 10 | `core/builtin_ls.c` | list directory contents in long format | `ls -l -b`; needs `ls -b` |
| 39 | who | yes | no | user | 120 | `core/builtin_utmp.c` | show who is logged on | utmp, `-a -b -q -u`, format work |
| 40 | pinky | no | no | user | 120 | `core/builtin_utmp.c` | lightweight `finger` for local users | utmp plus gecos |
| 41 | basenc | no | yes | text | 130 | `core/builtin_basenc.c` | encode or decode base16/32/64 and variants | base64, base32 and others in one |
| 42 | install | no | no | fs | 150 | `core/builtin_cp.c` | copy files and set mode and owner | copy plus mode/owner, `-d`, `-D`, `-t` |
| 43 | df | yes | no | system | 150 | `core/builtin_df.c` | report free space of file systems | `statvfs`, `/proc/mounts` or `getmntent`, `-P`, `-k` |
| 44 | du | yes | no | fs | 150 | `core/builtin_du.c` | estimate file space usage | tree walk, `-s -k -a -x -L -H`; uses the directory walker (`lib/walk`, see TODO.md "PLAN") |
| 45 | join | yes | takes two files, so no | text | 200 | `core/builtin_join.c` | join lines of two files on a common field | field joins, `-1 -2 -a -v -o -t -e` |
| 46 | stat | no | no | fs | 200 | `core/builtin_stat.c` | display file or file system status | many format escapes |
| 47 | fmt | no | yes | text | 200 | `core/builtin_fold.c` | reformat paragraph text | paragraph refill |
| 48 | od | yes | yes | text | 250 | `core/builtin_od.c` | dump files in octal and other formats | many formats and offsets |
| 49 | pr | yes | yes | text | 250 | `core/builtin_pr.c` | paginate or columnate files for printing | pagination, columns, headers |
| 50 | dd | yes | yes (reads stdin to stdout) | fs | 250 | `core/builtin_dd.c` | convert and copy a file | `if/of/bs/count/skip/seek/conv` |
| 51 | csplit | yes | no | text | 250 | `core/builtin_csplit.c` | split a file by context lines | regex and line splits, uses the `text/dfa` engine |
| 52 | numfmt | no | yes | text | 300 | `core/builtin_numfmt.c` | convert numbers to and from human-readable form | SI/IEC scaling |
| 53 | ptx | no | yes | text | 300 | `core/builtin_ptx.c` | produce a permuted index of file contents | permuted index |
| 54 | stty | yes | no | tty | 400 | `core/builtin_stty.c` | change and print terminal line settings | termios flags and `-a` formatting; the biggest one |
| 55 | dircolors | no | no | tty | 250 | `core/builtin_dircolors.c` | set up the colours `ls` uses | `LS_COLORS` database parse; only useful if `ls` gets colour |
| 56 | stdbuf | no | no | proc | n/a | - | run a command with modified stdio buffering | needs `LD_PRELOAD`; doesn't fit a builtin |
| 57 | chcon | no | no | fs | n/a | - | change the SELinux security context of files | SELinux; skip |
| 58 | runcon | no | no | proc | n/a | - | run a command in a given SELinux context | SELinux; skip |

(`dir` and `vdir` are 10 lines only once `ls` gains `-C` and `-b`.)

## Utilities that share one source file

"goes in" names the source file. A file marked with several utilities holds all of them, each behind its
own `#if BUILTIN_<NAME>` switch (see the plan in `TODO.md`: one builtin map, one switch per builtin).
Files that already exist are listed as `(existing)`.

| file | utilities | what they share | ~LOC saved |
|---|---|---|---|
| `builtin_id.c` (existing) | whoami, logname, groups | `getpwuid`/`getgrgid`/`getgrouplist` plumbing that `id` already has; `whoami` is `id -un`, `groups` is `id -Gn` | 60 |
| `builtin_uname.c` (existing) | arch | `arch` is `uname -m` | 10 |
| `builtin_env.c` (existing) | printenv | the `environ` walk and name lookup that `env` does | 20 |
| `builtin_cp.c` (existing) | install | `install` is a copy plus mode and owner; `cpmv` already does the copy | 80 |
| `builtin_ls.c` (existing) | dir, vdir | table rows with preset flags (`ls -C -b`, `ls -l -b`), like `gunzip` on `gzip`; needs `ls -C` and `-b` first | 20 |
| `builtin_digest.c` (existing) | b2sum, md5sum.textutils | a table row each, if the hash library has BLAKE2 | 15 |
| `builtin_sysinfo.c` | hostid, nproc, sync | tiny one-call utilities; no real shared code, just fewer files | 30 |
| `builtin_basenc.c` | base64, base32, basenc | one codec parameterized by alphabet and block size; `base64` and `base32` are aliases of `basenc` | 150 |
| `builtin_expand.c` | expand, unexpand | tab-stop list parsing and column tracking | 70 |
| `builtin_cksum.c` | cksum, sum | the same "read, update a checksum, print count" loop | 40 |
| `builtin_chown.c` | chown, chgrp | `chgrp` is `chown` with only a group; one owner-parsing function and one directory walk | 90 |
| `builtin_mknod.c` | mkfifo, mknod | `mkfifo` is `mknod p` plus `-m` | 30 |
| `builtin_utmp.c` | who, users, pinky | one utmp reader and one login-time formatter | 120 |
| `builtin_nice.c` | nice, nohup | the "adjust the process, then exec the command" helper that `timeout` already has | 50 |
| `builtin_tac.c` | tac, shuf | both slurp all input and permute an array of line offsets | 50 |
| `builtin_join.c` | comm, join | both merge two sorted inputs by key with a shared two-file reader and line comparison | 80 |
| `builtin_fold.c` | fold, fmt | both wrap text to a width; `fmt` adds paragraph refill | 70 |
| `builtin_stty.c` | stty, tty | both need the terminal descriptor and `termios`; `tty` is about 25 lines on top | 25 |

**Kept apart:** `dd`, `od`, `pr`, `csplit`, `tsort`, `numfmt`, `ptx`, `du` and `df` share no meaningful code
(`du` only the directory walker, which lives in `lib/walk`, not in a builtin file); `stdbuf`, `chcon` and
`runcon` are not worth building.

**Directory walker.** `chown`, `chgrp` and `du` (and later `install -d`) walk a tree the way `rm -r`, `chmod -R`,
`cp -R`, `find` and `ls -R` do today. The plan in `TODO.md` ("Part C") replaces those hand-rolled walks with
one `lib/dirlist` + `lib/walk`; do these utilities after it.
## What to do in order

1. **Batch 1, about 250 lines total:** `sync arch hostid yes tty nproc whoami logname printenv`. They need
   no new infrastructure.
2. **Batch 2:** `tac truncate mkfifo nice nohup seq cksum`. These are the POSIX ones plus the scripting
   staples.
3. **Batch 3, the text filters:** `expand unexpand fold comm base64 base32`. They share the filter
   framework, so each goes quickly after the first.
4. **Batch 4, ownership and disk:** `chown chgrp pathchk` (sharing `chmod`'s directory walker), then
   `du df`.
5. **Big ones last:** `dd od pr join csplit stty`.

## Open questions

- **`b2sum`:** can piggyback on `digest` only if its hash library has BLAKE2. Not checked.
- **`seq`:** it may be in POSIX.1-2024. Not sure.
