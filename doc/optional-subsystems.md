# Optional subsystems: alias, history, job control

A builtin that is switched off (`-DBUILTIN_<NAME>=OFF`) should take its machinery with it. Today it does not,
for three of them. `BUILTIN_ALIAS=OFF` still compiles and runs the alias code in the parser and the input
layer, `BUILTIN_HISTORY=OFF` still compiles `src/history/` and the line editor still reads history, and there
is no switch at all that turns job control off. This document records what each of the three consists of, what
"off" has to mean, and a graded set of refactorings.

**Status.** Done: alias scenario 1, the builtin map (build step 2 in "Order and dependencies"), history H1, the
`keep` call-site guards and directory entries in the map; see "What was done". Open: history H2, job control,
alias scenario 3, and the remaining findings in "Other code that is useless when its builtin is off".

The three are ordered by size: alias (small, a few files), history (medium, one directory and the line
editor), job control (large, it sits under every `fork` the shell does).

Written against `main` at `5eb453ae`; the "What was done" and "Other code" sections against the tree after the
alias change. All counts come from `grep` over `src/`; line counts are from `wc -l`.

## Principles that apply to all three

1. **One switch, one effect.** `BUILTIN_<NAME>=OFF` removes the builtin *and* every function, variable, table
   row and call that exists only for it. A build with it off must not contain the symbols (check with `nm` on
   the object files in `libshell.a`).
2. **Precedent exists.** `BUILTIN_TRAP` already gates code outside the builtin: `#if BUILTIN_TRAP` in
   `src/sh/sh_exit.c`, `sh_loop.c`, `sh_sigignore.c` and `src/term/term_read.c`; those files include the
   generated `builtin_config.h`. The new gates follow that style.
3. **A module header with stubs, not scattered `#if`s.** Each subsystem gets (or keeps) a header in `src/`
   (`history.h` is the model: `src/history/*.c` plus `history.h`, with the command in `builtin_history.c`).
   When the switch is off the header turns the calls into constants (`#define alias_find(n, l) NULL`,
   `static inline` no-ops), so the callers need no `#if` at all.
4. **State in a struct, one extern.** The state of a subsystem is one `struct` with one `extern` instance
   (booleans as one-bit fields, see CLAUDE.md "Types"); no loose `extern` globals.
5. **One function per file** for new module code (`src/alias/alias_find.c`, ...). Existing files are not split
   as a side effect of a refactoring; move code when it has to move anyway.
6. **The build files follow.** Sources of a switched-off module are not compiled: the CMake source list drops
   the directory (the planned builtin map's `needs` column, see "What was done" below), and autotools drops the
   directory from `SUBDIRS` in `src/Makefile.in`.
7. **Check matrix.** For each subsystem: build with the switch on and off, run `tests/fixed.sh` and the
   `tests/posix` files that exercise it (tests skip themselves when the builtin is missing, as
   `tests/builtin-dirstack.sh` does), build under ASan, and compare `nm` and `size` of the two builds.

---

## 1. Alias

### What exists

| piece | where | notes |
|---|---|---|
| alias list `parse_aliases`, `parse_findalias()` | `src/parse/parse_findalias.c`, declared in `parse.h` | list head lives in the parser; the list is edited in `builtin_alias.c` |
| `struct alias`, inline `alias_code()` | `parse.h` | |
| six globals: `source_alias_blank`, `source_alias_popped[8]`, `source_alias_poppedat[8]`, `source_alias_npopped`, `source_tokskips`, `source_tokskips_set` | defined in `src/source/source_alias.c`, `extern`s in `source.h` | `source_skips` (counted per character in `source_skip.c`) exists only for them; `source_tokskips` is set in `parse_word.c` |
| push, pop, active, reset | `src/source/source_alias.c` (4 functions in one file) | `source_alias_pop()` is declared by hand inside `source_peekn.c` |
| `parse_alias_subst()`, `parse_alias_next()` | static in `src/parse/parse_gettok.c` | only used there |
| `p->alias_ok`, `P_NOALIAS` | `parse.h`; set in `parse_init.c`, `parse_case.c` (4x), `redir_parse.c` | |
| alias callers outside the parser | `exec/exec_type.c`, `builtin/builtin_command.c` (`type`, `command -v`), `builtin/builtin_trap.c` (re-parse a trap body when any alias exists) | |
| `SOURCE_ALIAS` frames | `source_peekn.c` (pops used-up frames), `source_skip.c` (excluded from the `set -v` echo) | |
| the builtins | `src/builtin/builtin_alias.c`: `alias`, `unalias`, validity, new/insert/remove/search/print | |

### Findings

- **A real bug.** The table of popped aliases holds 8 entries and silently drops the rest, so the "an alias
  does not expand inside its own replacement" rule stops working once more than 8 aliases have been popped
  while one word is read. This hangs shish:

  ```sh
  alias a1=a2 a2=a3 a3=a4 a4=a5 a5=a6 a6=a7 a7=a8 a8=a9 a9=a10 a10=a9
  a1          # shish: loops forever; bash: "a9: command not found"
  ```

  A cycle among the first few aliases (`alias a9=a10 a10=a9; a9`) is caught correctly.
- **`BUILTIN_ALIAS=OFF` builds and works** (`alias` then gives "No such file or directory"), but the parser
  and input layer still carry everything: `parse_findalias`, `source_alias_*`, the `SOURCE_ALIAS` loop in every
  `source_peekn()`, a state reset per token, a counter increment per character, and the alias lookup in `type`
  and `command -v`, all working on a list that can never be filled.

### Scenarios

**Scenario 1: one state struct, one narrow interface, one switch** (about 100 changed lines in 8 files, no
file moves). Fixes the hang, removes the loose externs, makes the switch real. Do this first. **Done**, see
"What was done".
- `source.h` gets one `struct alias_scan` (list head, trailing-blank flag as `unsigned blank : 1`, the popped
  aliases, the token-start counters) and one `extern`. The seven externs go away.
- The popped aliases become a heap list instead of an 8-entry table: no limit, no hang.
- `source_alias_pop()` is declared in `source.h`; the hand-written declaration in `source_peekn.c` goes.
- `#if BUILTIN_ALIAS` in five places: the bodies of `parse_findalias.c` and `source_alias.c`, the
  `SOURCE_ALIAS` loop in `source_peekn.c`, the call from `parse_gettok.c`, and (as a macro that is `NULL`
  when off) `parse_findalias()` for `exec_type.c` / `builtin_command.c` / `builtin_trap.c`.
- Regression case in `tests/fixed.sh` for the 10-alias chain.

**Scenario 2: everything in `builtin_alias.c`** (about 400 lines in one file, state `static`, accessor
functions instead of externs). **Not recommended:**
- at least six public functions in one file break the one-function-per-file rule;
- the parser and the input layer would depend on a builtin file; `builtin.h` does carry helper prototypes
  (`builtin_rm_tree()`, `chmod_symbolic()`), but a parser including `builtin.h` inverts the layering;
- the parser-only tools `shformat` and `shparse2ast` link no builtins and would need stubs.

**Scenario 3: a module `src/alias/` with `src/alias.h`** (about 350 mostly mechanical moved lines in about 16
files; do it after scenario 1, it is then a move). Follows the `history` pattern, so there is no
`builtin_alias.h`:

```
src/alias.h          struct alias, struct alias_scan + extern, inline alias_code(), prototypes,
                     and the #if BUILTIN_ALIAS stub macros
src/alias/alias_scan.c    the state definition
alias_find.c (was parse_findalias)   alias_active.c   alias_push.c   alias_pop.c   alias_reset.c
alias_subst.c (was static parse_alias_subst)          alias_next.c (was static parse_alias_next)
alias_valid.c   alias_new.c   alias_insert.c   alias_remove.c   alias_search.c   alias_print.c
src/builtin/builtin_alias.c   only builtin_alias(), builtin_unalias() and the help strings
```

With the switch off, `alias.h` defines `alias_subst`, `alias_next` and `alias_find` as constants, so the
parser, `source_peekn.c`, `exec_type.c`, `builtin_command.c` and `builtin_trap.c` carry no alias code; the
`source_skips` increment is guarded the same way. Build files: a new `src/alias/Makefile.in` and a
`configure.ac` entry (like `history`); CMake already globs `src/*/*.c`.

---

## 2. History

### What exists

`src/history/` (12 files, 665 lines with `history.h`), one function per file. Clients outside the module:

| client | use |
|---|---|
| `src/sh/sh_loop.c` | `history_init()` when the shell is interactive; `history_add()` after every interactive command |
| `src/sh/sh_main.c` | `history_shutdown()` at exit |
| `src/sh/sh_init.c` | includes `history.h` only |
| `src/term/term_ansi.c` | up/down arrow call `history_prev()` / `history_next()` |
| `src/term/term_vimode.c` | vi `k` / `j` do the same |
| `src/term/term_search.c` | Ctrl-R search reads the session ring, the file map and the cursor directly (`history_session*`, `history_file_entry`, `history_decode`, `history_cursor`, `history_pending`) |
| `src/builtin/builtin_history.c` | `history_print()`, `history_clear()` |

Not a client today but one tomorrow: `fc` (`BUGS: fc-missing`) is a front end to the same store, and `set -H`
(`histexpand`, `BUGS: set-histexpand-unimplemented`) would need it.

### What "off" must mean

`BUILTIN_HISTORY=OFF` removes the `history` builtin, the in-memory ring and the mmap'd file, the `history_*`
calls in `sh_loop.c` / `sh_main.c`, and the browsing and searching in the line editor. The editor itself stays:
arrow keys, `Ctrl-R` and vi `k`/`j` just do nothing (or the key is not bound), and `term_search.c` is not
compiled. `fc` is gated on the same switch ("`fc` requires history").

### Scenarios

**Scenario H1: stubs and guards, `src/history/` stays a directory** (about 60 changed lines). **Done**, see "What was done". `history.h`
gets the `#if BUILTIN_HISTORY` real prototypes / `#else` constant stubs (`history_init()` and
`history_shutdown()` become empty inlines, `history_add()` a macro that evaluates its arguments' side effects
away); the eight call sites then need no `#if`. `term_search.c` and the bodies of `term_ansi.c` /
`term_vimode.c` history keys are wrapped in one `#if BUILTIN_HISTORY` each. The sources of `src/history/` drop
out of the CMake source list and of the autotools `SUBDIRS`. Checked with `BUILTIN_HISTORY=OFF` that no
`history_*` symbol is left.

**Scenario H2: H1, plus decouple the editor from the store** (about 150 changed lines). Today `term_search.c`
reaches into the history internals (the extern state in `history.h`). Give the editor a small read interface
instead (`history_count()`, `history_get(index, &line, &len)`, `history_cursor_*`), make the state one struct
with one extern, and then `term_search.c` needs only the interface. This is also what `fc` needs, so it is the
natural time to write `fc` on top.

**Open question (decide before starting):** `-H` / `histexpand` is accepted and does nothing today. Either it
stays a no-op that exists in both configurations, or it moves behind the same switch.

---

## 3. Job control

The biggest of the three, because job control is not a feature layered on the shell: `src/job/` is also the
shell's *process table*. Every external command, every pipeline member, every `cmd &` and every command
substitution child goes through `job_new` / `job_fork` / `job_wait`.

### What exists

`src/job/` is 15 files, 1255 lines with `job.h`. Clients outside it, by symbol group (from `grep`):

| group | symbols | clients |
|---|---|---|
| **A. the process table** | `job_new`, `job_fork`, `job_wait`, `job_signal`, `job_free`, `job_bypid`, `job_find`, `job_first`, `job_recall`, `job_discard`, `job_bgpid` (`$!`), `job_wait_interruptible` / `job_wait_sig` | `exec_program.c` (8 forks, 5 waits, for every external command), `exec_command.c`, `eval_pipeline.c` (7 forks), `eval_node_bgnd.c`, `eval_subshell.c`, `eval_tree.c`, `eval_cmdlist.c`, `expand_command.c`, `expand_param.c` (`$!`), `sh_forked.c`, `sh_loop.c` |
| **B. interactive job control** (`set -m`, "monitor mode") | process groups (`pgrp`), `job_terminal`, `job_pgrp`, `job_foreground`, `job_terminal_init`, stop detection, `job_banner`, `job_print`, `job_clean`, `job_update`, `job_dump`, `opts.monitor` | `exec_program.c` (9 `monitor` checks), `sh_main.c`, `eval_simple_command.c`, `eval_pipeline.c`, `exec_command.c`, `fdstack_data.c` (`job_resume_stopped`), `builtin_jobs.c` |
| **C. the SIGCHLD self-pipe** | `job_sigfd[2]` | `sh_main.c` (4), `term_read.c` (9: the editor's `select()` loop wakes on it), `builtin_trap.c` (3) |

Builtins that depend on the table: `jobs`, `fg`, `bg` (B), `wait` (A: `job_find`, `job_first`, `job_recall`,
`job_wait`), `kill` (`%N` operands use `job_find`), `timeout` (`job_bypid`, `job_wait`, `job_quiet`), `trap`
(an interruptible `wait`, the self-pipe). `builtin_uncompress.c` holds SIGCHLD around archive reads.

### What "off" must mean

If every builtin that needs the table is disabled (`jobs fg bg wait`, and `kill`'s `%N` form), job control is
turned off completely: `src/job/` is not compiled and the clients use the plain POSIX interface (`fork`,
`waitpid`, `kill`, `sigaction`) for processes and signals. `set -m` has no effect (or is refused; decide, see
below), there is no process-group handling, no terminal hand-over, no banners and no SIGCHLD handler.

The two layers are not the same thing, though. **A** is needed by any shell that forks; **B** only by an
interactive one. So there are really two switches: B can go without A going (`jobs fg bg` off), A can only go
when nothing wants the table (`wait` off too).

### Scenarios

**Scenario J1: make B optional only** (about 120 changed lines, no file moves; the cheapest real step).
Introduce one derived macro, `JOB_CONTROL` = `BUILTIN_JOBS || BUILTIN_FG || BUILTIN_BG`, and a
`sh_monitor()` that is the constant 0 when it is off. The nine `monitor` checks in `exec_program.c` and the
others collapse to constants and the compiler removes the process-group and terminal code. The files that
only exist for B (`job_banner.c`, `job_print.c`, `job_clean.c`, `job_update.c`, `job_foreground.c`,
`job_dump.c`) drop out of the build. A stays as it is.

**Scenario J2: split `src/job/` into two modules** (about 300 moved lines; the real refactoring).
- `src/proc/` + `proc.h`: layer A under a neutral name (`proc_new`, `proc_fork`, `proc_wait`, `proc_find`,
  `proc_recall`, ...). Everything that forks uses `proc_*`.
- `src/job/` + `job.h`: layer B only (`jobs`, `fg`, `bg`, the banners, the terminal), calling `proc_*`.
- Clients then include `proc.h`; only `builtin_jobs.c`, `sh_main.c`, `term_read.c` and the few B call sites
  include `job.h`. The self-pipe (C) moves with whichever side owns the SIGCHLD handler.

**Scenario J3: make A optional too** (large; after J2). `proc.h` has two implementations behind one
interface:
1. the table version (today's code), used when `wait` / `kill %N` / `timeout` / `jobs` need it;
2. a plain POSIX version for builds that need none of them: `proc_fork()` is `fork()`, `proc_wait()` is a
   `waitpid()` loop, `$!` is one stored pid, no table, no SIGCHLD handler (children stay unreaped until
   waited for, which is what `waitpid` expects). With it, `src/job/` and the table code are not compiled
   and `job_sigfd` disappears from `term_read.c`.

Risks specific to J3: a background child that finishes before it is waited for stays a zombie (acceptable,
it is waited for at exit or by the next `waitpid(-1)`); `wait` with no operands and `$?` after `wait $!`
rely on the table, so they are only available in the table build; the `timeout` builtin kills the *current
child* through `exec_child_pid`, which is the plain-POSIX shape already.

**Open questions (decide before starting):**
- With job control compiled out, does `set -m` fail with an error, or silently do nothing? (POSIX only
  requires job control with the User Portability option; both are conforming.)
- Is `wait` allowed to be off at all? It is a required POSIX utility; a build without it is a deliberate
  size-over-conformance choice, like `BUILTIN_TRAP=OFF`.
- Do the interactive extras (`term`, the self-pipe wake-up) get their own switch, or do they simply follow
  `JOB_CONTROL`?

---

## What was done

### Alias scenario 1 (commit `b1a7904c`, `fixes/323`)

- `struct alias_scan` in `source.h` holds the list, the popped aliases, the token counters and the two one-bit
  flags; its one instance is defined in `source_skip.c` (beside `source_skips`, so it exists with the switch off,
  because `builtin_trap.c` reads `alias_scan.list`). The seven loose externs and `parse_aliases` are gone.
- Popped aliases are a heap list freed by `source_alias_reset()`: the 10-alias cycle ends with "not found".
- With the switch off `parse_findalias()` is `NULL`, `source_alias_reset()` is a no-op, and the pop loop in
  `source_peekn()` and `parse_alias_subst()` are compiled out.
- Left for scenario 3: `parse_alias_next()` and `p->alias_ok` still run, `source_skips` is still counted per
  character, and the struct stays in `source.h`.

### The builtin map (build step 2)

`src/builtin/builtins.map` is the one list of builtins, one line per `BUILTIN_<NAME>` switch:

```
# name   file                   tier  needs                                        [keep]
alias    builtin_alias.c        m     ../source/source_alias.c,../parse/parse_findalias.c
mv       core/builtin_cp.c      x     core/builtin_rm.c
set      builtin_set.c          m     -                                            keep
```

| piece | what it does now |
|---|---|
| `cmake/Builtins.cmake` | reads the map; `ALL/MINIMAL/DEFAULT/EXTRA_BUILTINS` come from the tier column; `builtin_source()` and its special cases are gone |
| `CMakeLists.txt` | every source the map names is removed from the `src/*/*.c` glob and comes back only through an enabled line |
| `configure.ac` + `builtins.awk` | the m4 name lists come from the map instead of `ls`/`grep`; `BUILTIN_OFF` lists the sources no enabled builtin needs |
| `src/*/Makefile.in` (6) | `MODULES` is the wildcard minus the `BUILTIN_OFF` files of that directory; the old name filter in `core/extra/filter` is gone |
| `tests/builtin-map.sh` | drift guard (6 checks): table, map and source tree agree, every named file exists |

- **`needs`** also carries non-builtin sources, which is how `BUILTIN_ALIAS=OFF` now drops `source_alias.c`,
  `parse_findalias.c` and `builtin_alias.c` from the library.
- **`keep`** marks a builtin whose file is called from outside `src/builtin/`; it is always compiled and the
  switch only removes the table row. The matrix found five (`command`, `eval`, `source`, `set`, `trap`); four were
  guarded afterwards (see "The `keep` call sites"), only `set` is left.
- Found and fixed on the way: `tr` had no switch in CMake (it could not be enabled); `continue` had none (always
  on, now tier `m`); `cp` alone failed to link (`builtin_rm_tree()` lives in `builtin_rm.c`, now in its `needs`);
  the autotools `ALL_BUILTINS` list contained `error`.
- Check run: default set, one switch flipped at a time (extras ON, the rest OFF), build, link and `type NAME`.
  82 of 84 switches ran (`digest` needs `third_party/`, `compress`/`uncompress` need libarchive); all link
  after the `keep` column (re-checked for the four that lost it). Not yet a CTest: it needs a build per switch (see Part B's `tests/builtin-matrix.sh`).
- Autotools was checked up to the generated `MODULES` lists and a full default build. It also needs a manual
  `./config.status` first, because `configure` never runs it (`BUGS: configure-skips-config-status`).

### History H1

- `history.h` is the stub header: with `BUILTIN_HISTORY` on it declares the store as before; off, `history_init`,
  `history_shutdown`, `history_add`, `history_prev` and `history_next` are `((void)0)` macros, so `sh_loop.c`,
  `sh_main.c`, `term_ansi.c` and `term_vimode.c` carry no `#if`. `term.h` does the same for `term_search()`.
- The `history` line of the map names the 12 `src/history/*.c` files and `src/term/term_search.c` in `needs`, so
  `BUILTIN_HISTORY=OFF` does not compile them: no `history_*` or `term_search` symbol is left in `libshell.a`, the
  shell links and runs, arrow keys, `Ctrl-R` and vi `k`/`j`/`/` do nothing.
- Decision taken for the open question: `-H` / `histexpand` stays a flag that is accepted and ignored in both
  configurations.
- Not done (H2): `term_search.c` still reads the store's globals directly, and `fc` does not exist.

### The `keep` call sites

`command`, `eval`, `source` and `trap` are no longer `keep`; only `set` is (`sh_main.c` parses the command line with
`set_apply`/`set_longopts`, so the option machinery cannot go while a shell exists).

- `exec_command.c`: the "status of the last command is not a builtin error" test is `exec_status_passthru()`, which
  compares against `builtin_source`/`builtin_eval`/`builtin_trap` only when their switch is on.
- `eval_simple_command.c`: the `command exec` shortcut sits inside `#if BUILTIN_COMMAND`.
- `src/trap.h` (new): the prototypes of the `trap_*` calls that evaluator, jobs, line editor and three builtins
  made (each file used to declare them by hand), with constant stubs when `BUILTIN_TRAP` is off
  (`trap_run_pending()`, `trap_snapshot_save()` ... and `trap_run_count`/`trap_run_sig` as `0`). The `#if BUILTIN_TRAP`
  islands in `sh_loop.c`, `sh_sigignore.c`, `term_read.c` and the first one in `sh_exit.c` became plain calls.
- `lib/wait/waitpid_nointr.c` used to read `trap_signaled` from `builtin_trap.c`: a library file depending on a
  shell symbol. The flag is now defined in the library file and `builtin_trap.c` declares it `extern`.
- Check: `BUILTIN_{COMMAND,EVAL,SOURCE,TRAP,HISTORY,ALIAS}=OFF` each build, link and run.

### Directory entries in `needs`

A `needs` entry ending in `/` stands for every `.c` file of that directory (`file(GLOB)` in CMake, `ls` in
`builtins.awk`, the `OFF_HERE` rule in `text/*/Makefile.in`). Used for the text engines:

```
awk   extra/builtin_awk.c   x   ../../text/awk/,../../text/dfa/
sed   filter/builtin_sed.c  x   ../../text/sed/,../../text/dfa/
grep  expr  nl                  ../../text/dfa/
```

A default build now has 0 `awk_*` and 0 `sed_*` symbols (it keeps `dfa_*` because `expr` is in the minimal tier);
each of `awk`, `sed`, `grep`, `expr`, `nl` switched on alone links and runs with only the engines it needs.

---

## Other code that is useless when its builtin is off

Found by listing every function in `src/` and `text/` whose callers are all inside `src/builtin/`, then every
function a builtin file defines that is called from outside it, and by `nm` on a switched-off build.

### A. Helpers only a builtin calls (dead when it is off, but in an always-compiled file)

| builtin(s) | helper(s) | file |
|---|---|---|
| `dump` | `fd_dumplist`, `vartab_dump`, `job_dump`, `fdtable_dump`, `fdstack_dump` (and `var_dump`, reached only through `vartab_dump`) | `fd/`, `vartab/`, `job/`, `fdtable/`, `fdstack/` |
| `set`, `export`, `readonly`, `local` | `vartab_print` (and `var_print` behind it) | `vartab/vartab_print.c` |
| `export`, `readonly` | `var_copys` | `var/var_copys.c` |
| `fdtable` | `fd_print` | `fd/fd_print.c` |
| `getopts`, `unset` | `var_unset` (dead only when both are off) | `var/var_unset.c` |
| `history` | `history_clear`, `history_print` | `history/` (in H1) |
| `jobs`, `kill`, `wait` | `job_find`; `wait` alone: `job_first`, `job_recall` | `job/job_find.c`, `job_free.c` |
| `source` | `sh_popargs` | `sh/sh_popargs.c` |
| `type` | `exec_type` | `exec/exec_type.c` |
| `timeout` | `job_quiet`; `exec_child_pid` is also set by `exec_program.c` and read by `trap` | `job/`, `exec/` |
| `wc cut paste tr uniq` | `sh_utf8` | `sh/sh_utf8.c` |

**Fix:** one line per helper in the map's `needs` column of the builtin that owns it (the helper's file is then
compiled only while it is on). Where two builtins share one, the helper goes in the `needs` of both. This is
mechanical now that the map exists; each helper file needs a check that nothing else in `src/` includes it.

### B. The filter module and the text engines

- `text/awk`, `text/sed` and `text/dfa` used to be globbed into `libshell.a` unconditionally. **Fixed** with
  directory entries in `needs` (see "What was done"); `dfa` is shared by `expr`, `grep`, `nl`, `sed` and `awk`.
- `src/filter/` (`filter_run`, `filter_in_*`, `filter_copy`, `filter_opt_count`) is *not* removable by `needs`: the
  evaluator itself uses it (`eval_pipeline.c` runs filter builtins in-process, `fd/fd_filter.c`,
  `fdstack/filter_in_ready.c`). Only helpers no evaluator file calls (`filter_opt_count`, `filter_in_peek_lines`,
  `filter_copy`) could move behind the filter builtins' `needs`.

### C. Subsystems with no switch of their own

| subsystem | used by | state |
|---|---|---|
| `src/history/`, line-editor history | `history`; `sh_loop`, `sh_main`, `term_*` | switchable (H1 done); H2 open |
| `src/job/` | `jobs fg bg wait kill timeout trap`, every fork | section 3 |
| `src/term/` (line editor) | interactive shell only; `read` and `trap` wake-ups | no builtin owns it. It would need a switch of its own (an interactive-less build) to drop `term/`, `prompt/` and the self-pipe |
| `src/prompt/` | `PS1`/`PS2` for the interactive shell and the parser hook | same as `term/` |
| `src/debug/`, `src/trace/` | `dump`, `DEBUG_OUTPUT` | already switched. `tree_print*` cannot be: `set`, `trap`, `dump` and xtrace call it (the `NO_TREE_PRINT` option that pretended to was removed) |

### D. Not useless, although the builtin is off

- **`hash`**: `exec_hash` is the command lookup cache, used by every command and `type`; `set -h` and the resets
  in `eval_function.c` keep it consistent. With `hash` off only the builtin (its listing and `-r`) goes. The cache
  stays.
- **`ulimit`, `times`, `umask`, `getopts`, `local`, `read`, `cd`**: self-contained; dropping the file is enough.
  (`umask` is also restored in `sh_pop.c` through the `sh->umask` field, which is a plain `mode_t`, not code.)
- **Option flags** that exist for a builtin (`set -o histexpand|ignoreeof|nolog|vi`) are one-bit fields; they
  cost nothing and stay.

### E. Accepted but never acted on

`set -H` (`histexpand`), `-o ignoreeof`, `-o nolog` and `-o vi` are parsed and stored and nothing reads them
(`vi` reaches `term_vimode.c` only through the editor). They belong with the history decision in the open
question of section 2.

---

## Order and dependencies

1. **Alias scenario 1** (done). Then alias scenario 3, now that the build-file support exists.
2. **The builtin map** (done, see "What was done"): "this module's sources are compiled only when its switch is
   on" is the `needs` column, now also with directory entries. **Part B** (a switch per builtin inside shared
   files, `-Wundef`, the matrix test as a CTest) and the helper findings A are still open.
3. **History H1** (done), then H2 together with `fc`.
4. **Job control J1**, then J2, then J3 only if the size win is wanted: J2 already gives the clean layering.

The three do not depend on each other. Shared infrastructure: the `#if BUILTIN_<NAME>` stub style, the
`builtin_config.h` include, and the check matrix above.
