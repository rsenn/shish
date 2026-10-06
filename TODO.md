# TODO / Roadmap

Leverage-sorted list of what's still open. Fixed work lives in `git log` and
`fixes/*.patch`, not here — this file only tracks what's left to do. See
`BUGS` for confirmed, reproducible defects with repro steps.

---

## MAIN QUEST — POSIX conformance and memory safety

Everything else in this file is secondary to this until it's done. The
site's own pitch (`docs/conformance.html`) says shish is "proof-of-concept
quality" and targets POSIX "and nothing else" — the job is closing the gap
between that claim and `tests/posix`, without introducing memory corruption.
Two stages, done in order, plus one requirement that runs continuously
underneath both:

1. **Stage 1 — the shell language itself.** Everything `tests/posix`
   measures that is not a builtin: signal disposition, which errors
   must exit the shell, expansion/quoting/parsing, control flow and
   exit status. This is Phases 1, 2, 4, 5 below. Land this first — several
   builtin failures (e.g. `set -o` option names, `trap` printing) are thin
   wrappers over language-level state that Stage 1 fixes anyway.
2. **Stage 2 — utilities/builtins.** `alias`, `read`, `set`, `option`
   and the rest — Phase 3 below. Independent, per-builtin fixes; start once Stage 1's
   language-level failures stop shadowing them.
3. **Ongoing, throughout both stages — memory safety.** A frequent
   ASan+UBSan build is a gate, not a one-off cleanup pass: every change
   in Stage 1/2 must be re-verified under
   `-fsanitize=address,undefined` before being counted as done, the
   same way `tests/fixed.sh`/`ctest` already are. See "Memory safety"
   below for what is open there and how to run it.

**Non-goal, decided 2026-09-02:** bash's `var+=value` append-assignment
(not in POSIX; confirmed unimplemented -- `x=a; x+=b` parses `x+=b` as
a command name and fails). This is the reason libtool's `ltmain.sh`
can't run under shish, but libtool/libtool-generated scripts are not a
target -- don't add `+=` (as an opt-in flag, the same way `-B`/`-H`
gate brace/history expansion, or otherwise) unless that changes.

The measurable target for Stages 1-2 is `tests/posix` (yash's POSIX suite, 120 files).

### HIGH PRIORITY — PLAN (nothing implemented yet): one builtin map, one switch per builtin, one directory walker

Three changes that have to land in this order. Each ends with `ctest` green, the ASan build clean, and
`tests/fixed.sh` at its 9 known failures.

#### Part A - one name-to-file map replaces `builtin_source()` and the hand-kept lists (DONE: `src/builtin/builtins.map`, see `doc/optional-subsystems.md` "What was done")

**Today** four lists describe the same facts and drift apart (every shared file so far needed a
hand-written special case, and `dirs`/`popd`/`pushd` once linked wrong because of it):
- `cmake/Builtins.cmake`: `MINIMAL`/`DEFAULT`/`EXTRA`/`ALL_BUILTINS`, `builtin_source()` (probes
  `core/`, `extra/`, `filter/`, then the top directory; hard-codes `mv`->`builtin_cp.c`,
  `dirs`/`popd`/`pushd`->`builtin_dirstack.c`), the disabled loop that `REMOVE_ITEM`s a shared file
  even when a sibling is still enabled, and three "this builtin also needs that file" lines
  (`mv`->`builtin_rm.c`, `mkdir`->`builtin_chmod.c`, `uncompress`->`builtin_compress.c`).
- `src/builtin/builtin_table.c`: ~117 rows, one `#ifndef BUILTIN_X` default each, plus `extern help_*`
  and prototypes in `src/builtin.h`.
- `configure.ac`: `ALL_BUILTINS`/`EXTRA_BUILTINS` rebuilt from the directory layout with `ls`/`grep`.
- `src/builtin/*/Makefile.in`: `MODULES` matched by file name (`builtin_<name>.c`), which cannot
  express a file that holds several builtins.

**Plan.** One plain-text map, `src/builtin/builtins.map`, one line per builtin *name*:

```
# name            file                       tier  needs
cp                core/builtin_cp.c          d
mv                core/builtin_cp.c          d     core/builtin_rm.c
dirs              extra/builtin_dirstack.c   x
gzip              filter/builtin_compress.c  x
gunzip            filter/builtin_uncompress.c x    filter/builtin_compress.c
[                 core/builtin_test.c        d     -              macro=LBRACKET
md5sum.textutils  extra/builtin_digest.c     x     -              macro=MD5SUM_TEXTUTILS
```

- `tier` is `m` (minimal), `d` (default) or `x` (extra): it replaces the three CMake lists and the
  directory-derived `EXTRA_BUILTINS` in `configure.ac`.
- `needs` lists helper source files the builtin cannot link without; it replaces the three special
  cases above and the file-name probing.
- `macro=` overrides the switch name where the builtin name is not an identifier (`[`, names with `.`);
  the default is the name upper-cased.
- `builtin_source(OUT NAME)` becomes a lookup into variables CMake sets once from the map
  (`file(STRINGS ... REGEX)`, no external tools). The source set is *built up* from the enabled
  names (their file plus their `needs`) instead of added and then `REMOVE_ITEM`ed, which removes the
  disabled-sibling bug for good.
- `configure.ac` reads the same file with `m4_esyscmd` + `awk`; each `src/builtin/*/Makefile.in`
  `MODULES` comes from a generated `builtin_files.mk` instead of a `wildcard`.
- **Drift guard:** a CTest (`tests/builtin-map.sh`) checks that every `{"name", &fn` row of
  `builtin_table.c` has a map line and vice versa, every `builtin_*.c` is named by some line, and
  every file in a `needs` column exists.
- **Later, optional:** generate the table rows, `#ifndef` defaults and the prototypes/help externs from
  the same map (or from an X-macro `builtins.def` that CMake and `awk` can both read), so that adding
  a builtin means one map line and one source file.

#### Part B - every builtin gets its own preprocessor switch (also the existing ones)

**Status:** every file under `src/builtin/` that the map names is wrapped in `#if BUILTIN_A || BUILTIN_B`
(the switches whose line names it as source or in `needs`; generated from the map, `keep` files and
files that already had guards excepted). With every switch off each file compiles to an empty object.
Shared files with per-name guards inside: `dirs`/`popd`/`pushd`, `cp`/`mv`, `break`/`continue`,
`test`/`[` (`[` is `macro=LBRACKET` in the map). `-Wundef` for `src/builtin/` is not enabled: it
gives 166 warnings from `LINK_STATIC`, `WINDOWS_NATIVE` and `GREP_USE_SYSTEM_REGEX` in `lib/` and
none from `BUILTIN_*`; `tests/builtin-map.sh` checks every `BUILTIN_<NAME>` under `src/` against
the map instead. Still to run: the full `tests/builtin-matrix.sh` (one build per name) and the
ASan/UBSan gate. The matrix is not a CTest case; run `sh tests/builtin-matrix.sh [name ...]` after
touching a builtin, and `WITHOUT="test" sh tests/builtin-matrix.sh '['` for a tier m sibling on its own.

**Decided, not planned:** `builtin_digest.c` stays one switch (`BUILTIN_DIGEST`) for its eight names.
It is table-driven (`digest_algos[]`), and the switch also gates the hash-library download in
`cmake/Digest.cmake`; splitting it would touch CMake, `configure.ac` and the tests for no gain.
`compress` and `uncompress` are table-driven over libarchive the same way and stay family switches.

**Today** most names already have their own `BUILTIN_<NAME>` (`cp` and `mv`, `break` and `continue`, `dirs`,
`popd` and `pushd` are separate switches in separate table rows), but several families share one:
`BUILTIN_COMPRESS` guards nine names (`gzip` ... `zstd`), `BUILTIN_UNCOMPRESS` eleven (`gunzip`, `zcat`,
`unxz`, ...), `BUILTIN_DIGEST` six (`md5sum` ... `sha512sum`), and `[` rides on `BUILTIN_TEST`. Inside the
source files nothing is guarded per builtin: `builtin_cp.c` always compiles `cp` and `mv` together,
`builtin_dirstack.c` all three of its builtins, and so on; the file is just included or not.

**Plan.**
- `builtin_config.h` already defines `BUILTIN_<MACRO>` as 0 or 1 for every name in the map; keep that,
  and compile with `-Wundef` for `src/builtin/` so a misspelled switch is an error, not a silent 0.
- In every file, each builtin's entry function, its `help_*` string and the helpers only it uses sit
  inside `#if BUILTIN_<NAME>`; helpers shared by several builtins of the file sit inside
  `#if BUILTIN_A || BUILTIN_B`. A one-builtin file gets the same guard, so the rule has no exceptions
  and a file with every switch off compiles to an empty object.
- The map decides whether a file is compiled at all (any name of it enabled); the `#if`s decide what is
  inside it. The table rows in `builtin_table.c` already use `#if BUILTIN_<NAME>`; they gain the
  per-name switch for the family rows (`gzip`, `xz`, `zstd`, ... each its own).
- The families dispatch on `argv[0]`/a name table: that table is filtered the same way, and the
  compression backends (`zlib`, `bzip2`, `lzma`, `lz4`, `zstd`, `lzo`) are linked only when a name that
  needs them is on, which also cuts the library dependencies of a small build.
- **Matrix test:** `tests/builtin-matrix.sh` configures the minimal set plus *one* extra builtin at a time
  (`-DBUILTIN_<NAME>=ON`), builds, runs `shish -c 'type NAME'`, and reports any configuration that
  fails to compile or link. It is the check the `dirs`/`popd`/`pushd` link error would have tripped.
- **Existing shared files to convert:** `core/builtin_cp.c` (`cp`, `mv`), `core/builtin_test.c` (`test`,
  `[`), `builtin_break.c` (`break`, `continue`), `extra/builtin_dirstack.c` (`dirs`, `popd`, `pushd`),
  `filter/builtin_compress.c` (9 names), `filter/builtin_uncompress.c` (11 names),
  `extra/builtin_digest.c` (6 names), `builtin_type.c`; then the ~75 single-builtin files by rote.

#### Part C - one recursive directory walker instead of six hand-rolled ones

**Builtins that walk a directory tree today** (found with `grep -l 'opendir\|readdir' src lib text`):

| where | what it walks | order | symlinks | extras it needs from a walker |
|---|---|---|---|---|
| `core/builtin_rm.c` `builtin_rm_tree()` (also called by `mv`) | `rm -r`, cross-device `mv` | children first, then the directory | never followed (`lstat`) | `-f` (ignore ENOENT), `-v`, `-i` prompts *before* descending and *before* removing, "a refused child keeps its parents" |
| `core/builtin_chmod.c` `chmod_path()` | `chmod -R` | directory first, then children | command-line operand followed, nested ones skipped | `-v`, `-c`, `-f`, stat before and after |
| `core/builtin_cp.c` `cpmv_dir()` | `cp -R` | directory created first, then children | `-H` / `-L` / `-P` policy | holds the `DIR*` open while recursing (one fd per level) |
| `core/builtin_ls.c` `ls_dir()` | `ls -R` | listing of a directory, then each subdirectory | `lstat`, never followed | reads all names first, sorts, then closes the directory |
| `extra/builtin_find.c` `find_recursive()` | `find` | directory, then children | `-L` follows, with an ancestor `(dev, ino)` chain against loops | predicates run per entry, `-prune`-like skipping |
| `src/term/term_complete.c` | tab completion | one level only, not recursive | - | wants the same "read a directory into a list" step |

Utilities from `doc/coreutils.md` that need the same walk: `chown -R`, `chgrp -R`, `du` (also `install -d`
in a later pass).

**Plan.** Two layers, both in `lib/` (generic: only `stralloc`, `byte`, `str` and `<dirent.h>`/`<sys/stat.h>`,
no shell dependency; `lib/` is compiled into `libowfat.a`, so a build without a walking builtin does not
link a byte of it):

1. **`lib/dirlist.h` + `lib/dirlist/dirlist_read.c`**: read one directory into an array of
   `{name, len, d_type}`, optionally sorted, then `closedir()` - so a walk holds one descriptor at a time,
   not one per level (the `cp -R` weakness). `ls`, `term_complete` and the walker use it.
2. **`lib/walk.h` + `lib/walk/walk_path.c`** (an `fts(3)`/`nftw(3)` equivalent without libc's, which dietlibc
   and mingw lack): a callback walker.

```c
enum { WALK_PHYS = 0, WALK_LOGICAL = 1, WALK_COMFOLLOW = 2,   /* lstat / follow all / follow operands */
       WALK_DEPTH = 4,  WALK_XDEV = 8,  WALK_SORT = 16 };      /* post-order, stay on device, sort names */
enum { WALK_PRE, WALK_POST, WALK_FILE, WALK_ERR, WALK_CYCLE };  /* what the visit is about */
enum { WALK_GO, WALK_SKIP, WALK_STOP };                         /* visit result: go on / not into this one / abort */

struct walk_ent { const char* path; const char* name; size_t len; int depth, phase, err; struct stat st; };
struct walk { int flags, maxdepth; int (*visit)(struct walk*, struct walk_ent*); void* ctx; };
int walk_path(struct walk*, const char* root);
```

   - one growing path buffer (as `builtin_rm_tree()` does) instead of one buffer per level;
   - the ancestor `(dev, ino)` chain `find` already has moves in, reporting `WALK_CYCLE`;
   - `WALK_PRE` returning `WALK_SKIP` is what `rm -i` ("descend into directory?" refused) and `find -prune`
     need; a child's `WALK_GO` count lets `rm` keep a directory whose entry was refused;
   - errors are visits (`WALK_ERR` with `errno`), so each builtin keeps its own wording and `-f` handling.
   - A walk keeps no per-directory state: `cp -R` derives the destination as `dst + (path + rootlen)`.

**Migration order** (each step keeps that builtin's tests green and is its own patch):
`lib/dirlist` + a unit test (`tests/walk_test.c`, built like `arena_test`) over a generated tree with a
symlink loop, a permission-denied directory and a 3000-deep chain -> `rm` / `builtin_rm_tree()` (also
carries `mv`) -> `chmod -R` -> `find` -> `cp -R` -> `ls -R` (uses `dirlist` only; its per-directory
header/sort/`total` logic stays) -> `term_complete`. New `chown`/`chgrp`/`du` start on the walker.
**Not walkers:** `mkdir -p` and `rmdir -p` follow the *components of one path* and stay on `lib/path`.

**Rejected:** importing `../c-utils/lib/{dir,rdir}`. `rdir_read()` yields a flat pre-order stream of paths;
it has no post-order, no skip, no symlink policy and no loop guard, which is exactly what `rm`, `find`
and `cp` need. Its `dir_open`/`dir_read`/`dir_type` layer (a `FindFirstFile` backend for Windows) is worth
borrowing only if the mingw build ever needs a walker that `<dirent.h>` cannot give it.

#### Order of work
1. Part A (map, CMake and `configure.ac` read it, drift guard test) with no source file touched.
2. Part B on the shared files first, then the rest; the matrix test lands with the first converted file.
3. Part C: `lib/dirlist`, `lib/walk`, then the builtins in the order above.
4. Only then the new coreutils of `doc/coreutils.md`, in the groups it lists (several of them share a
   file, which Part A makes cheap).

### Next after the plan above: remaining `ls` options (`BUGS: ls-missing-options`), then `fc` (`BUGS: fc-missing`)

Done: `type` (all operands, not-found status), `test` (`-ef`, 3-argument `-a`/`-o`, status 2 for bad
integers), `cd` (`x/..` components). Still open from them: `BUGS: type-a-unimplemented`,
`cd-operand-count-and-e-option`; `break`/`continue` inside `eval` and `ln` (one operand, `-L/-P`, keeps
existing destination without `-f`) are fixed.

**Next in line**, in the order they were weighed (see `BUGS` for the repro of each):
`ls-missing-options`, `fc-missing`, then the rest of `BUGS` by repro.

### Where it stands (2026-10-03)

Failing cases per file, everything except the `sig*` family (7 of the 58 files that run
have any; the full `ctest` run has 12 failing tests, 3 of them `tests/*.sh` that need a GNU `date`
or the other known causes in `BUGS`):

```
4 alias-p  61/65    1 param-p 53/54    2 quote-p 33/35    1 case-p 51/52
1 input-p  10/11    1 option-p 74/75   1 simple-p 33/34
```

- `option-p:121` and `simple-p:172` fail on purpose (`BUGS: posix-suite-intentional-deviations`).
- `kill1/2/3-p` pass now: the harness (`tests/posix/run-test.sh`) discards the host shell's own
  job notice for a signaled testee, and a non-interactive shell no longer prints one for a
  signal sent on purpose.
- Every runnable `sig*-p` file passes 180/180. The `*3/4/7/8` files, `sigstop`, `sigtstp`,
  `sigttin`, `sigttou` and six others (`bg fg job kill4 testtty wait`) skip themselves: they
  need a controlling terminal.
- **Do not trust a signal-file number from a busy machine.** The same binary scored
  `sigterm1-p` 36/180 in one run and 177/180 in the next; measure on an otherwise idle
  machine and re-measure before concluding anything
  (`BUGS: signal-tests-vary-with-machine-load`).

### How to measure

```sh
# one file (testee path MUST be absolute)
sh tests/run-tst.sh "$PWD/build/x86_64-linux-gnu/shish" tests/posix exec-p.tst

# whole suite
(cd build/x86_64-linux-gnu && ctest)

# scoreboard from the .trs files the run leaves behind
cd tests/posix && for f in *.trs; do
  t=$(grep -Ec '^%%+ (PASSED|FAILED|SKIPPED):' "$f")
  x=$(grep -Ec '^%%+ FAILED:' "$f"); s=$(grep -Ec '^%%+ SKIPPED:' "$f")
  [ "$x" -gt 0 ] && printf '%4d %-14s %d/%d\n' "$x" "${f%.trs}" "$((t-x))" "$t"
done | sort -rn

# what a family is actually failing on
grep -h -E '^%%+ FAILED' tests/posix/sig*.trs | sed -E 's/.*: SIG[A-Z]+ //; s/ \(.*//' \
  | sort | uniq -c | sort -rn
```

Every phase below ends the same way: rerun the named files, update the table above,
remove the closed `BUGS` entry, add `fixes/NN` + a case in `tests/fixed.sh`.

---

### Phase 1 [Stage 1: language] - signal disposition (done)

`sigint6`/`sigquit6` pass 180/180: `trap - SIG` in an interactive async shell now restores the default action.

### Phase 2 [Stage 1: language] - diagnostics

shish prints `file:LINE:COL: msg` where the line number is one too high (the parser has already
advanced) and omits the offending name. `echo ${x?boom}` on line 2 reports `:3:1: boom`; bash
reports `line 2: x: boom`. `$LINENO` itself is correct. Fix with, and verify against,
`lineno-p.tst` (3/3) and `BUGS: error-message-line-number-off-by-one`.

---

### Phase 3 [Stage 2: builtins/utilities]

1. `alias` (61/65) - the 4 left are in `BUGS: alias-substitution-remaining-cases`.
2. `read` (28/28) and `option` (74/75) are done apart from the intentional deviations.
3. **`set`** - `-b` is accepted and shown in `$-` but has no effect (`BUGS: set-notify-no-effect`);
   `-v` echoes input lines (`BUGS: set-verbose-partial`); `ignoreeof`, `nolog`, `vi` are accepted
   but do nothing (`BUGS: set-o-ignoreeof-nolog-vi-has-no-effect`, `set-histexpand-unimplemented`).

---

### Phase 4 [Stage 1: language] - expansion and parsing

1. `quote-p` (33/35) - `BUGS: quote-backslash-escaping-broken`.
2. `param-p` (53/54, only `:82` left) - `BUGS: param-expansion-pattern-removal-broken`.
3. `simple-p` (33/34), `tilde-p`, `cmdsub-p`, `comment-p` are done (`simple-p:172` is intentional:
   `BUGS: posix-suite-intentional-deviations`); `case-p` (1) - `BUGS: case-pattern-bracket-quote-stripping`.

---

### Phase 5 [Stage 1: language] - control flow and exit status

1. `trap-p` (37/37) is done.
2. (done: `break`/`continue` inside `eval`, via the `E_EVAL` frame flag.)
3. `input-p` (1) - the shell reads ahead inside a command substitution
   (`BUGS: input-not-read-line-wise`).
4. `function-p`, `pipeline-p`, `for-p`, `exec-p`, `builtins-p` are done.

---

### Phase 6 [Stage 1+2] - what is not being measured at all

1. **The `%REQUIRETTY%` files** (44: the `sigttin`/`sigttou`/`sigtstp`/`sigstop` `*3-p`/`*7-p`/`*8-p`
   combos, `kill4-p`, `bg-p`/`fg-p`/`job-p`, `testtty-p`, `wait-p`) need a real controlling terminal.
   `-DDO_PTY_TESTS=ON` runs them under `tests/pty-run.c` (a single-file POSIX-`pty` wrapper):
   **10/44 pass**, 34 fail. Some are ordinary conformance gaps (`sigcont3-p`'s 3 output
   mismatches), but most `*3-p`/`*7-p`/`*8-p` combos plus `wait-p`/`kill4-p` hang until the 60s
   `pty-run` alarm - a real job-control defect, since their `kill`-driven `*4-p` siblings pass in
   1-2s. One case is traced: `BUGS: wait-interrupted-by-trap-hangs`; the rest need per-case
   bisection (`BUGS: job-control-real-terminal-hangs-vs-kill-driven-ok`). Re-run via:
   ```sh
   cmake -S . -B build/x86_64-linux-gnu -DDO_PTY_TESTS=ON
   cmake --build build/x86_64-linux-gnu -j
   cd build/x86_64-linux-gnu
   NAMES=$(grep -l '%REQUIRETTY%' ../../tests/posix/*.tst \
           | xargs -n1 basename | sed 's/\.tst$//' | tr '\n' '|' | sed 's/|$//')
   ctest -R "posix/(${NAMES})\.tst\$" -j4
   ```
2. **`tests/yash` (119 files) is off by default**: `arith-y` and `while-y` hang, none isolated
   (`BUGS: yash-suite-other-hangs`). Isolate one hang per session; each is likely its own bug.
3. `grouping-p.tst:34` is flaky (2 of 12 runs): a race between a subshell's background writer and
   the FIFO read after it (`BUGS: grouping-p-tst-flaky`).
4. The harness leaves `tests/posix/tmp.NNNNN/` behind on every hard failure. Clean them up and
   make the harness remove its own.
5. `tests/fixed.sh` gives different results on a default build and with every builtin
   (`BUGS: fixed-sh-assumes-optional-builtins`, `fixed-sh-remaining-failures-after-sigchld-fix`,
   `fixed-sh-fails-under-non-mmap-build`).

---

### PLAN (alias scenario 1 and the builtin map are done) - make alias, history and job control really optional

Details, inventories and the graded scenarios are in `doc/optional-subsystems.md`. Short form:

- **alias** (`BUILTIN_ALIAS=OFF` still leaves the alias code in the parser and the input layer, and a latent hang:
  an alias chain of 10 ending in a cycle loops forever because the "popped aliases" table holds 8 entries).
  **Scenario 1 (DONE):** one `struct alias_scan` + one extern instead of seven loose globals, a heap list instead
  of the 8-entry table, `#if BUILTIN_ALIAS` at five choke points, regression case in `tests/fixed.sh`.
  **Scenario 3 after it:** module `src/alias/` + `src/alias.h`, one function per file, `builtin_alias.c` keeps only
  `alias`/`unalias` (no `builtin_alias.h`; `history.h` + `src/history/` is the precedent).
- **history:** H1 is DONE (stub header, `needs` entries drop `src/history/` and `term_search.c`). H2 (a read
  interface for the editor, with `fc`) is open; `-H`/`histexpand` stays an ignored flag.
- **job control:** the biggest. `src/job/` is also the shell's process table (every external command, pipeline
  member and `&` goes through it). J1 make interactive job control optional (`JOB_CONTROL` macro,
  `sh_monitor()` constant), J2 split `src/job/` into `src/proc/` (process table) and `src/job/` (jobs/fg/bg,
  terminal, banners), J3 a plain POSIX `proc` implementation when nothing needs the table.
- **Other dead code found** (`doc/optional-subsystems.md`, "Other code that is useless when its builtin is off"):
  builtin-only helpers in `src/` (list in A) are still open. DONE: directory entries in the map's `needs` column
  (the `text/` engines follow awk/sed/grep/expr/nl) and the `keep` call-site guards (`src/trap.h`; only `set`
  stays `keep`). `src/filter/` is part of the evaluator and stays.
- **Prerequisite for the directory-level switches (done):** the builtin map (Part A above), so that a module's sources
  are compiled only when its switch is on (CMake source list and autotools `SUBDIRS`). Decisions to take first
  are listed in the "Open questions" of the document (`set -m` when job control is compiled out; whether `wait`
  may be off; `-H`/`histexpand`).

### PLAN (nothing implemented yet) - busybox-style applet mode: `exec -a cat shish` runs only `cat`

`ln -s shish cat; ./cat file`, or `(exec -a cat ./build/.../shish file)`, runs the `cat` builtin and exits with its
status; no shell is started. The decision is made from the basename of `argv[0]` alone.

**Which names are applets.** Every builtin whose source is in `src/builtin/*/*.c` (`core/`, `extra/`, `filter/`,
the coreutils-style utilities). The shell-special and shell-state builtins in `src/builtin/*.c` (`cd`, `export`,
`set`, `alias`, `eval`, ...) are never applets: they only mean something inside a shell. The map already carries
this split (the `file` column of `src/builtin/builtins.map` starts with `core/`, `extra/` or `filter/`), so no new
table is needed. The path is stripped (`/usr/bin/cat` -> `cat`); a leading `-` stays with the shell (login name).
A name that is not an applet (`sh`, `shish`, anything else) starts the shell as today.

**Where.** `sh_main.c`, right after the environment is imported into the root vartab (`env` reads it) and before
option parsing: look the basename up in `builtin_table` (new `builtin_applet(name)`, a variant of
`builtin_search()` that accepts only entries whose map file is not top-level), call `fn(argc, argv)` with the
original `argv`, flush `fd_out`/`fd_err` and `exit(status)`. In applet mode `SHELL` is not set, no options are
parsed, no history and no interactive setup run.

**What must stay live.** The shell runtime (fd table, vartab, builtin table): builtins write through `fd_out`, and the
builtins that run another command (`timeout`, `env`, `xargs`, `exec`, `command`) call `exec_command()`, which picks
builtin / function / program by table lookup. `timeout` already forks a builtin without exec
(`exec_command(..., X_NOWAIT)`, `builtin_timeout.c`) and kills the child by pid, so it needs no re-exec.

**What breaks, and the fix.** `exec_program.c` re-runs an `ENOEXEC` script with `execve("/proc/self/exe", sargv, ...)`
and `sargv[0] = argv[0]`; a script named like an applet (`cat`) would turn the re-executed shish into that applet.
`sargv[0]` becomes the fixed name `sh` (`$0` comes from `sargv[1]`, so nothing visible changes).

**Open points.** A map flag for "never an applet" among the `src/builtin/*/` names if one turns out to need the
shell (`dirs`/`pushd`/`popd` keep a stack in the shell: they are in `extra/` but must be excluded); `--help` text
comes from the builtin's `help_*`; `shish cat args` (explicit form) is deliberately not offered, it collides with a
script named `cat`.

**Tests.** `tests/applet.sh`: `exec -a cat "$SHISH_SELF"` and a symlink in a temp dir, stdin through `cat`,
exit status, a builtin with options (`wc -l`), `timeout 1 sleep 5` as an applet, a non-applet name starting a shell,
and the script-named-`cat` case for the `sargv[0]` fix.

### LOW PRIORITY - more utilities as builtins (plan only, nothing implemented)

Two prioritized lists of programs that are not builtins yet, each with size estimates, POSIX status, whether it can
be a filter, a category, and the source file it should share with its relatives:

- `doc/coreutils.md` (58 entries from `coreutils.unimplemented`): start with the one-liners
  (`sync arch hostid yes tty nproc whoami logname printenv`, about 250 lines), then `tac truncate mkfifo nice
  nohup seq cksum`, then the text filters (`expand unexpand fold comm base64 base32`), then `chown chgrp pathchk du
  df` on the directory walker, and `dd od pr join csplit stty` last.
- `doc/util-linux.md` (80 entries from `util-linux.unimplemented`): Linux-only, mostly root-only, so every one is off
  by default behind its own `BUILTIN_<NAME>` and compiled only where the headers exist. Start with the scripting
  staples (`rev mcookie mesg setsid mountpoint fallocate flock namei isosize`), then the process wrappers
  (`chrt taskset ionice choom uclampset setarch`, `prlimit` on `ulimit`), then namespaces (`unshare nsenter pivot_root
  switch_root`, testable with `unshare -Ur`), then a shared `lib/coltab` table printer and the `ls*` listing tools.
  Not worth building: `su sulogin getty agetty fsck* mkfs.*`.

**Prerequisites, in this order** (all are the HIGH PRIORITY plan above): the builtin map (Part A) so a file can hold
several builtins, the per-builtin switches (Part B) so each of them can be left out, and the directory walker (Part C)
for `chown`, `chgrp`, `du`, `hardlink` and `switch_root`. Do not start a utility from either list before Part A.

**Cross-cutting decisions to take first** (also in the "Open questions" of both documents):
- whether Linux-only builtins belong in the tree at all (the plan assumes a busybox-like single binary is the goal);
- `configure` checks and the `syscall()` fallbacks for calls without a libc wrapper (`ioprio_set`, `sched_setattr`,
  `pivot_root`), including dietlibc and musl, and keeping these files out of the Windows and wasm builds;
- tests as a normal user in a user namespace that skip themselves when the builtin or the privilege is missing.

### Loop findings that need a design decision (not fixed, see `BUGS`)

- **`unset -v 1x` status** (`unset-invalid-name-accepted`): shish prints the error and returns 0. bash
  returns 1 and continues; dash exits the non-interactive shell because `unset` is a special builtin.
  A plain `return 1` lands on dash's behavior here (the shell exits), so pick one deliberately.
- **Empty file operand gets no header from `head`/`tail`** (`head-no-header-for-empty-file`): the empty
  source is skipped inside `filter_in_ready()` (`src/filter/filter_in_ready.c`) before `head`/`tail` see it.
  Reporting it means `filter_in_peek()` has to hand out "a file was opened and is empty" (`newfile` set,
  0 bytes), which `grep`, `cat`, `tr`, `compress` and `uncompress` would all have to treat as "nothing yet",
  not as the end of input. Smallest idea: a separate `in->empty` bit that only `head`/`tail` read, set where
  `filter_in_ready()` closes a source that still has `newfile` set.

- **`"$@"""` with no positional parameters gives 0 fields, POSIX (and bash, dash) give 1**
  (`quoted-at-then-empty-quotes-drops-field`; also `"$@"''`): `expand_is_empty_at()`
  (`src/expand/expand_args.c`) treats "only empty literal chunks plus a quoted `$@`" as zero fields. The
  empty chunk after a plain `"$@"` is created by the parser when the quote state switches
  (`src/parse/parse_string.c`: a new `N_ARGSTR` whenever `flag & S_TABLE` differs from `p->quot`), so it
  looks the same as a written `""`. Smallest idea: set a flag bit on an `N_ARGSTR` only where the source
  has an explicit empty quote pair (`""`, `''`), and let `expand_is_empty_at()` ignore only unmarked
  empty chunks. Needs a `tests/posix` run of `quote-p`, `param-p` and `field-p`-style files, since the
  word parser is shared by everything.

- **`break`/`continue` at the top of a file run with `.`** (`source-break-continue-no-op`): bash and dash leave
  the calling script's loop, shish ignores it. `builtin_source()` pushes an `E_ROOT` frame with its own
  `setjmp`, which `eval_jump()` treats as a boundary. `eval` got past this with the `E_EVAL` flag, but a
  jump that skips the source frame would also skip its cleanup (`sh_popargs`, `source_popfd`, `eval_pop`),
  the kind of leak `eval_jump()`'s comment describes. Smallest idea: let `eval_jump()` stop at the source
  frame, longjmp into `builtin_source()` with a "propagate break/continue by N levels" code, and have it
  clean up and re-issue `eval_jump()` from the caller's frame.

- **Here-documents in `tree_cat()`** (`tree-cat-mangles-here-documents`): `redir_addhere()` fills the
  body into the redirection node when it is read, in place of the delimiter word, so the printer has
  neither the delimiter nor the "was it quoted" bit and prints `cat <<"body\n"`. A correct printer needs
  (1) a delimiter chosen at print time (one that does not occur as a line of the body), (2) the quoting
  bit kept on the node (quoted delimiter = no expansion in the body), and (3) the body emitted after the
  next newline of the output, not at the redirection: `cat <<EOF | grep x` has the body after the whole
  line. That is a queue of pending bodies flushed by `tree_catseparator()`, i.e. a change to the output
  order of the whole printer, which `set`'s function dump, `trap -p` and `shformat` all share.

- **Forked function children lose their output inside `$( )`**
  (`timeout-function-output-not-captured-by-command-substitution`): it is not specific to `timeout`.
  `f() { echo hi; }; x=$(f & wait)` also gives `[]` (bash: `[hi]`), while `x=$(f | cat)` works. A command
  substitution collects stdout in a `stralloc`; pipeline members get a real pipe when they fork, but an
  async job and `timeout`'s `exec_command(..., X_NOWAIT)` fork with fd 1 still bound to the `stralloc`.
  The fix belongs where a pipeline stage materializes its pipe (fdstack), reused for X_NOWAIT/`&`
  forks: trace with `SHISH_TRACE=fd,fdtable,exec` on `x=$(f & wait)` and compare `fdtable.exec.table`
  with `fdtable.exec.fds` in the child.

### `BUGS` <-> conformance-gap map

**Explains a scoreboard number (fix these as part of Stages 1-2):**

- `signal-tests-vary-with-machine-load` -> Phase 1 (`sig*-p`).
- `error-message-line-number-off-by-one` -> Phase 2 (`lineno-p`).
- `alias-substitution-remaining-cases`, `set-notify-no-effect`, `set-verbose-partial`,
  `set-o-ignoreeof-nolog-vi-has-no-effect`, `set-histexpand-unimplemented` -> Phase 3.
- `quote-backslash-escaping-broken`, `param-expansion-pattern-removal-broken`,
  `case-pattern-bracket-quote-stripping` -> Phase 4.
- `input-not-read-line-wise` -> Phase 5.
- `posix-suite-intentional-deviations` stays as it is.
- `yash-suite-other-hangs`, `grouping-p-tst-flaky`, the three `fixed-sh-*` entries -> Phase 6.

**Real bugs, but not counted in the `tests/posix` scoreboard** (`type-unknown-name-silent` is the
HIGH PRIORITY task above; fix the rest opportunistically):
`eval-lineno-imprecise-inside-function`,
`no-tree-print-option-is-a-noop`,
`cfg-cmake-mingw-silently-builds-native`, `eval-node-bgnd-silent-on-fork-failure`,
`builtin-cp-sh-hangs`, `quoted-at-then-empty-quotes-drops-field`.

**Memory safety, not conformance** - under "Memory safety" below:
`asan-leak-residue-not-fully-triaged`, `ubsan-buffer-op-proto-function-type-mismatch`.

### Memory safety [ongoing, both stages] - ASan+UBSan as a recurring gate

A build that has to be run frequently (every fix in Stages 1-2, not just periodically) so a
language/builtin fix doesn't trade a conformance failure for a corruption bug:

```sh
cmake -B build/asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan
(cd build/asan && ASAN_OPTIONS=detect_leaks=0 ctest)    # ASan/UBSan abort = immediate hard failure
```

Open under this build, from `BUGS`:

1. `asan-leak-residue-not-fully-triaged` - 2 allocations per parsed function definition, and the
   process-lifetime function and variable state.
2. `ubsan-buffer-op-proto-function-type-mismatch` - `lib/buffer.h`'s `buffer_op_proto` cast onto
   libc `read`/`write` is UB by the letter of the standard but not fixable without wrapping two
   libc functions everywhere for no observable effect; not planned to change. The two real
   mismatches in unused `lib/buffer/` glob-compiled dead code are left alone per this repo's
   "don't touch unused `lib/` code unasked" standard.
3. **Goal 4 below**: the repeated per-scope saves are a corruption risk whenever a new
   in-process scope is added.

---

## Everything below this line is lower priority than the MAIN QUEST above.

---

## Goal 3 (secondary) — arena allocator for the AST (`lib/arena` exists; `text/` uses it, `src/` does not yet)

`src/tree.h`'s AST is a graph of individually `malloc()`'d nodes
(`tree_newnode()`) plus separately `malloc()`'d string buffers hanging off
several of them — one `malloc`/`free` pair per node, even though a tree's
real lifetime is always "parse it all at once, evaluate, throw the whole
thing away" (`sh_loop.c`). `lib/arena.h` is a generic bump allocator (pluggable
source, `arena_tell()`/`arena_rewind()` for nested lifetimes, `arena_grow()` for the newest
allocation only); only `text/dfa` and `text/awk` use it so far, nothing in `src/` does.

Design decisions already worked out (full reasoning in git history —
2026-07-23/24 commits):

- **One arena with marks, or a stack of arenas.** Every independent
  parse-evaluate-free scope (`sh_loop.c`, `builtin_eval.c`,
  `builtin_source.c`, `builtin_expr.c`, `prompt_parse.c`,
  `builtin_trap.c`'s inline parse) nests strictly via ordinary call-stack
  recursion — shish is single-threaded, so arenas never need to overlap
  without nesting. Push one per scope; `arena_reset()`/`arena_free()` it
  wherever `tree_free()` is called today; `arena_tell()`/`arena_rewind()`
  gives the same nesting inside a single arena.
- **`tree_free()` mostly disappears, not just changes signature.** Most of
  its current call sites just free a subtree still inside the current
  statement — those calls simply go away, since the dead nodes just wait
  for the enclosing arena to reset. Only the handful of true scope
  boundaries above get an `arena_reset()`/`arena_free()` call instead.
- **Two things can't live in the transient arena:** function bodies and
  trap bodies, since both must outlive the statement that defines them.
  Trap bodies already parse through their own independent `parse_init()`
  call, so they can just get their own dedicated, never-reset arena.
  Function bodies parse inline as part of the defining statement and are
  deep-copied into long-lived storage at adoption time by `tree_copy()`
  (`src/tree/tree_copy.c`, mirrors `tree_free()`'s per-kind switch). Once
  the arena lands, `tree_copy()` must switch from allocating loose nodes
  to bump-allocating into the function's own dedicated arena.
- **`stralloc` doesn't fit an arena** — it grows via `realloc()`, which
  can't work once other data has been bump-allocated after it. Two ways
  in: the parser keeps building in its one reusable heap `p->sa` and
  freezes the result with `arena_strndup()` (one copy, no waste); or, only
  when the string is the newest allocation, `arena_grow()` extends it in
  place and `arena_trim()` freezes it. `arena_grow()` never copies (that
  would leave a hole); it returns NULL and the parser falls back to `p->sa`.
  This covers the tree's own write-once-at-parse-time strings:
  `nargstr` (as its `strview view` overlay of `stra`), `nargparam.name`,
  `nfor.varn` and `nfunc.name` (populated once during parsing).
  `narg.stra` stays a real `stralloc` — it's populated later, at
  expansion time, not parse time. Packing a node and its string tightly
  adjacent in the arena is safe with no alignment padding, since
  `src/tree.h`'s node structs are already `__packed`.
- **Expansion results are a separate problem.** Words expand into `N_ARG` field nodes on the
  heap, not into the parse arena; see Goal 3b for the field-list redesign that follows this one.
- **Possible future: precompiled/cached AST on disk.** Serialize arena
  blocks with node pointers rewritten to offsets; on load, run one linear
  fixup pass turning offsets back into real pointers (structured like
  `tree_free()`'s own `switch(node->id)`) — after that, every existing
  tree-walking function works completely unmodified. A more invasive
  "offsets natively everywhere, zero-copy `mmap()`" design is possible but
  touches every tree-walking call site for a benefit unlikely to matter
  next to lexing/parsing cost.

---

## Goal 3b (secondary, after Goal 3) — word expansion into a field list, not N_ARG nodes (planned, not started)

Depends on Goal 3: nothing here starts until the parser allocates from an arena.

**What expansion is.** `src/expand/` works only on *words*: an `N_ARG` whose `list` holds
`N_ARGSTR`/`N_ARGPARAM`/`N_ARGCMD`/`N_ARGARITH` parts. The result is a chain of `N_ARG`
nodes, one per field, each carrying a growable `stralloc` (`narg.stra`). Consumers only ever
need `(char* s, len)` per field plus one bit ("keep this field even if empty",
`X_QUOTED|X_NOSPLIT|X_SPLIT`): `expand_argv()` for argv, `eval_for.c` for the loop
variable, `expand_vars()` for assignments (always exactly one field). Several entry points
(`expand_str/tostr/tosa/copysa/catsa`, `expand_range`; used by case, redirections, here-doc
delimiters, prompts, `${x:-word}` messages) want a single concatenated string and no fields at
all; `expand_copysa`/`expand_catsa` fake that with a `union node tmpnode` on the stack.

**Measured before Stage 1 (2026-09-20, `valgrind ./shish -c ...`, 1000 loop iterations, control loop
`[ $i -lt 1000 ]; i=$((i+1))` subtracted).** Per execution of one simple command:

| command                  | mallocs | bytes |
| ------------------------ | ------- | ----- |
| `:`                      | 5       | 214   |
| `: abcdefgh` (one word)  | 10      | 436   |
| `: a b c d e f g h`      | 45      | 1927  |
| `: $i`                   | 10      | ~440  |
| `: ${y:-z}`              | 13      | ~535  |

So a literal word cost **5 mallocs, ~215 B** (2 since Stage 1). `set -- $(seq 1 20000); : "$@"` adds
60,029 mallocs / 2.34 MB, about **3 mallocs and 117 B per field** for a ~6-byte string.

**Where the remaining mallocs per word go.**
1. The result is one `N_ARG` node (48 B packed + malloc header) and one `stralloc` buffer sized
   `len + len/8 + 30` (`lib/stralloc/stralloc_ready.c`), so short fields waste most of it.
   `expand_cat()` has seven copies of "`tree_newnode(N_ARG)` + `stralloc_init`".
2. Fields have no lifetime of their own: they live until `tree_free(args)`, after the command.

**Design (each step is its own change; Stage 1, skipping `tree_copy()` when no word needs `{`/`~` rewriting, is done).**

- **Stage 2: an append-only field sink instead of `union node**` cursors.**
  Findings that make this safe (read from `src/expand/`, 2026-09-20):
  - The output is only ever appended at the tail. `nptr` is not a splice point: it is either
    the caller's empty slot for the *first* node, or `&n` of a local holding the current tail
    (`expand_args`/`expand_arg` set `nptr = &n` after every part). The only rewrites are "replace
    the tail field by its glob matches" and IFS splitting (append). Brace expansion splices, but
    on the *input* copy. The returned `union node*` is just the new tail, used as the next cursor.
  - Field boundaries are made two different ways: `X_CATCLOSED` + `expand_cat_sibling()` for IFS
    splitting, and advancing the cursor to an empty slot (`nptr = &n->next`) for `"$@"`. The
    last field of a word is finalized (glob/unescape) in `expand_args()`, all others in
    `expand_cat_finish()`.
  - Input and output share `struct narg` but never mix: input uses `list`, output uses
    `flag`, `stra`, `next` (`id` and `list` are dead on results). Nothing feeds an output node
    back in as input. The `tmpnode` in `expand_copysa/catsa` and the temporary `N_ARG` in
    `expand_arith_expr()` exist only to have a `stralloc` to append to.
  - What consumers need: `expand_argv()` reads `stra.s` and the bits `X_QUOTED|X_NOSPLIT|X_SPLIT`
    (drop an empty field unless one is set); `eval_for` reads `stra`; `expand_vars` reads one
    `stra` per assignment (`var_setsa`); `eval_simple_command` hashes the first field. Nobody
    reads `X_LITERAL/X_GLOB/X_UNESCAPED` after the word is finished.

  Design:
  ```c
  struct xlist { stralloc blob; size_t* off; size_t n, a; stralloc cur; unsigned state; };
  void xl_cat(struct xlist*, const char*, size_t, int flags); /* = expand_cat's state machine */
  void xl_break(struct xlist*);   /* close the open field; the next cat opens a new one */
  char** xl_argv(struct xlist*);  /* NUL-terminated strings, argv-shaped; keep/drop already decided */
  ```
  Closed fields are NUL-terminated strings appended to one `blob`, with their offsets in one
  vector that `xl_argv()` turns into `char*` in place: the result *is* `argv`, with no
  per-field node or flag. The open field is built in `cur` (glob/unescape happen when it
  closes, in one place). The "keep an empty unquoted field" decision (`X_SPLIT`) is made when
  the next field opens or the word ends, so no flag survives. Single-string entry points
  (`expand_str/tostr/tosa/copysa/catsa`, case, redirections, prompts, `expand_arith_expr`)
  make `cur` the caller's `stralloc` and skip fields entirely, which deletes `tmpnode`.
  Two mallocs per command (blob, vector) instead of two per word, reusable per nesting level.
  `expand_cat()`'s state machine keeps its logic (it encodes many `fixes/NN`); only its state
  moves out of `narg.flag` into `xlist.state`, and the seven "new node" blocks become one
  `xl_open()`.
  Pointer stability: `blob` may move while building, so offsets are used until `xl_argv()`;
  afterwards it is fixed for the life of the command. Command substitution and function calls
  nest, but each level owns its own `xlist`, so nothing shares a moving buffer.
  Zero-copy option: a word that is one unquoted literal chunk without backslash or glob
  characters could put a pointer straight to the parse-tree string in the vector (parse strings
  are already NUL-terminated by `stralloc_nul()` in `parse_string.c`); valid while the tree is
  alive, i.e. for the command's duration.
  Not `stralloc[]` with flags in `.a`: `stralloc_ready()` treats `a != 0` as capacity, so a flag
  value there would look like allocated space; the existing alias convention is `a == 0`.
- **Rule: a field-rewriting step must run at finalization, never as a later pass.** Once the
  arena backs `blob` (Stage 4), any step that changes an already-closed field's size (glob is
  the existing example; a brace-on-expansion-result extension, which shish doesn't have --
  bash/dash/shish all leave `a='bl{a,e,i}h'; echo $a` unexpanded, confirmed 2026-09-21 -- would
  be another) can only extend that field in place while it is still `top` of the arena, i.e.
  before the next field opens. `xl_close()`/`expand_glob()` already run exactly there. A rewrite
  applied as a second pass over an already-built list, after a later field has been appended,
  finds the target is no longer `top`; `arena_grow()` correctly refuses it (see `lib/arena.h`),
  and the only fallback is an O(n) shift of everything after it in `blob` -- no cheaper than the
  `realloc()`-based copy the arena was meant to avoid.
- **Arena scoping (only once Goal 3 lands).** `blob` and the vector become arena allocations
  taken between `arena_tell()` and `arena_rewind()` in `eval_simple_command`, `eval_for` and
  `expand_vars`; with a contiguous sbrk arena (Stage 4) `blob` grows in place and `cur` can live
  at its tail. Until then plain `alloc()` is fine and is not an arena move.
- **Side effect on the parse tree.** `narg.stra` is used only by expansion, so `struct narg`
  drops its 24-byte `stralloc`: parse-time `N_ARG` nodes shrink from 48 to 24 bytes.
- **Stage 3 (optional): read-only tilde and brace.** Tilde can be applied when `expand_arg`
  sees a word's first literal chunk (`expand_tilde_lookup()` already returns home + prefix
  length). Brace expansion generates whole alternative words today, so it would become "expand
  the word once per alternative with chunk k replaced". After this `tree_copy()` is only used
  for function and trap bodies.
- **Stage 4 (optional): contiguous sbrk arena.** If `arena_brk` chunks are contiguous the arena
  can extend `end` instead of chaining; then `arena_grow()` never fails and `cur` could live at
  the arena tail (obstack style), removing the copy on close.

**Expected result:** ~2 mallocs per command instead of 2 per word, about length+1+8 bytes per
field instead of ~117 B, no `tree_free()` of expansion results, no `narg.stra`, and about ten
fewer `tree_newnode()` call sites.

**Rejected / not now.**
- *Growing `stralloc` inside the arena:* only the newest allocation can grow, and expansion
  interleaves nested expansions (`${x:-$(cmd)}`), so a heap `cur` is the robust choice.
- *Rewriting the escape scheme* (parser doubles glob characters, expansion undoes it with
  `X_LITERAL`/`X_UNESCAPED`/`X_PATTERN`): worthwhile but a separate, risky change.

**Risks.** Expansion is the most patched code in the tree (most `fixes/` touch it), so Stage 2
must not change behaviour. Before starting, capture a characterization test
(`tests/expand-fields.sh`: IFS variants, `"$@"` vs `$@`, `${x:+w}`, empty and quoted-empty
fields, glob with no match, `"a"'b'$c`) with expected values taken from bash and dash, and
compare the `tests/posix` and `tests/yash` pass counts before and after.

**How to measure.** Same commands as the table above (`valgrind ./shish -c '...'`, read
"total heap usage"); run ASan+UBSan too, for dangling fields after an `arena_rewind()`.

---

## Goal 4 (secondary, small) — one helper for an in-process scope's saves

`(...)` (`eval_subshell()`) and `$(...)` (`expand_command()`) run in this process, and
each repeats the same saves around its body: `fdstack_push`, `fd_state_save`/`restore`
(which also journals fds owned outside the scope), `vartab_push`, `sh_push`,
`exec_functions_save`, `trap_snapshot_save`, plus `sh_sigrestore()` and the job level.
`eval_pipeline()`'s in-process stages repeat a subset. A new in-process scope has to
copy all of them in the right order, and a missed one is a corruption bug.

Do: one `scope_enter()`/`scope_leave()` pair (a struct holding the six saves) used by
all of them, with the `jmpret` handling (`exit` from a real-signal trap re-runs
`sh_exit()`) in one place instead of three. Gate: `tests/fixed.sh`, the posix files and
the ASan build unchanged.

Known limitation: a foreground child that signals the shell by its literal pid inside a
scope is taken to mean "the subshell" (default action); bash sends it to the parent, whose
trap runs afterwards. The two cannot be told apart without separate processes.

---

## Goal 5 (secondary) — make the binary smaller (musl and dietlibc are the targets)

The pitch on the site is "a 185 KB shell". Every number below is
`stat -c%s` on a **stripped** binary, `MinSizeRel` (`-Os`), measured
2026-08-22 at `c44eab01`, gcc 16 / musl-gcc / diet-gcc on x86_64.

```
                         before 5.1   MinSizeRel today   hand-tuned ceiling
glibc, dynamic (default)     189312         142264            136152
musl, static                 237472         195160            191936
dietlibc, static          does not build    152072            149088
```

The middle column is what a plain `-DCMAKE_BUILD_TYPE=MinSizeRel` now
produces; the right one adds LTO, `--icf=all` and `-no-pie`, which are
still opt-in. The dietlibc row is not a typo -- a *static* diet build
undercuts the old *dynamic* glibc one.

### 5.1 Remaining opt-in build flags

Not in the numbers above: LTO (`-DENABLE_LTO=ON`, worth ~8%), `--icf=all`
(needs gold or lld), and `-no-pie` (drops `.rela.dyn`, at the cost of
ASLR for the executable). Stacked on the glibc dynamic build, the tuned
result is 136152 bytes (vs 142264 for plain `MinSizeRel`).

Notes from measuring:

- `--icf=all` needs gold or lld. **gcc `-flto` + `ld.lld` is broken**
  (lld cannot read GCC bitcode: `undefined symbol: main`), and it fails
  *at configure time*, so every `check_include_file` silently reports
  "not found" and the build then dies somewhere unrelated. Use gold
  with gcc; lld only with clang.
- `-DMINSIZE_STRIP=OFF` turns off the post-link `strip` of `.comment`,
  `.note*`, `.eh_frame`, `.eh_frame_hdr`.

### 5.2 Help and usage text: ~13 KB of a 136 KB binary

`.rodata` is 18662 bytes, and the 38 `help_*` strings are 10234 of
them -- 55%. On top: 848 bytes of usage strings and a 1760-byte
`builtin_table` in `.data.rel`. Roughly 10% of a tuned binary is text
that only `help` and usage errors ever print.

Wanted: `-DENABLE_HELP_TEXT=OFF` that nulls the `help`/usage fields of
`struct builtin`. Disabling the `help` *builtin* does not help today --
`builtin_table.c` names every `help_*` symbol, so they all link anyway.

Related, smaller: packing the two `char*` fields into offsets in one
string blob removes 56 relocations from `.data.rel.ro`.

### 5.3 Stop dragging libc subsystems in for one caller each

Measured in the musl static build:

| symbol pulled in | bytes | why | replacement |
|---|---|---|---|
| `pow` (+ libm) | 1916 | `A_EXP` in `expand_arith_binary.c:51` | integer `**` loop -- shell arithmetic is integer, so `pow()` is also a correctness hazard |
| `glob` + `do_glob` + `fnmatch_internal` | ~5200 | `expand_glob.c:61` | the shell already has `path_fnmatch` (1564 bytes); glob = readdir + that |
| `__qsort_r` | 991 | `term_complete.c:60`, sorting completions | insertion sort over a handful of names |

`lib/unix/glob.c` exists but is `#if WINDOWS_NATIVE` only, so every
Unix build takes libc's.

**Plan: an internal POSIX `glob`, and a `USE_LIBC_GLOB` option (2026-09-19).**
Policy for everything shish re-implements that libc also has: use libc's
where it costs nothing (dynamic linking) *unless* the libc function has no
`(ptr, len)` form and we need one — then the internal one is always used.
`fnmatch(3)` is the example: NUL-terminated only, so `path_fnmatch` stays
internal for `case`, `${x%pat}` and (Goal 6) regex bracket sets.

What each build carries for glob today (all measured 2026-09-19 except
musl, which is the figure above):

| Build | glob-related libc code in the binary |
|---|---|
| glibc, dynamic | 0 B (imports `glob64`/`globfree64`) |
| dietlibc, static | 3469 B: `glob` 1767 + `glob_in_dir` 751 + `globfree` 63 + `fnmatch` 888 |
| musl, static | ≈5.2 KB (table above) |
| glibc, static | 31 KB of code: `glob.o` 11.8 KB + `fnmatch.o` 19.2 KB (`libc.a` members; locale code they pull in not counted) |
| Windows | internal `lib/unix/glob.c`: 1480 B (mingw `-Os`, `WINDOWS_NATIVE` forced) |

1. **Write `lib/glob/` (POSIX flavour)**: `opendir`/`readdir` per path
   component + `path_fnmatch` (with `PATH_FNM_PERIOD`), same `glob()` /
   `globfree()` names and `gl_pathc`/`gl_pathv` as `lib/glob.h`, so
   `expand_glob.c` needs no change beyond the include it already selects
   with `HAVE_GLOB`. Estimate ≈150-200 lines, ≈1.2-1.8 KB (Windows
   version as the comparator; not written yet).
2. **CMake `USE_LIBC_GLOB`** = `AUTO` (default): ON for a dynamically
   linked libc that has `glob` (glibc dynamic: 0 B, and libc's glob is
   locale-aware), OFF for `LINK_STATIC`, dietlibc and Windows. When OFF
   the internal one is built and libc's `glob`/`fnmatch` are never
   referenced. Net saving when OFF: ≈2 KB dietlibc, ≈3.5 KB musl, ≈29 KB
   static glibc.
3. **Not identical to libc's glob — decide each, then test:** result
   order (`strcoll` vs bytes; same in the C locale), `[^…]` as negation
   (glibc accepts it, `path_fnmatch` takes only `[!…]`), backslash
   handling, leading-`.` rule, no-match/error return values and the
   `errfunc` callback. In exchange, pathname patterns and `case`/`%`/`#`
   patterns finally use **one** matcher (the open `case-pattern-bracket-quote-stripping` and
   `quote-backslash-escaping-broken` entries in `BUGS` are about expansion and quoting, not about
   which matcher runs, so this does not fix them).
4. **Verification:** a dev-only differential script over a fixture tree
   (dotfiles, brackets with classes, escaped metacharacters, symlinks,
   unreadable directories) comparing the internal backend with libc
   `glob64` in the C locale, plus `tests/` cases for what POSIX 2.13
   specifies (`/` never matched by `?`/`*`/`[...]`, leading period matched
   only explicitly, results in collation order, unmatched pattern left
   as is — `expand_glob.c` already handles the last). Must pass in both
   `USE_LIBC_GLOB` settings; ASan+UBSan gate as usual.
5. Interaction with Goal 7: with the internal `glob`, `?` and `[...]` in
   *pathname* patterns become UTF-8-aware for free once `path_fnmatch` is
   (M3), and the `setlocale` question in Goal 7 (C) disappears for
   static builds.

Other libc duplicates were surveyed and are **not** worth an option
(default glibc-dynamic build, unstripped relink, function sizes from
`nm -S`): `str_*`/`byte_*` are already macros over libc (`lib/str.h:59-61`,
`lib/byte.h:63-68`); thin wrappers `path_getcwd` 100, `path_readlink` 64,
`path_basename` 64, `mmap_read`+`mmap_read_fd` 219, `shell_gethostname`
88 total ≈0.6 KB of ≈107 KB of code; `path_canonicalize` 591 +
`path_realpath` 298 implement the *logical* path (`cd -L`) that
`realpath(3)` cannot; `shell_getopt_r` 350 is reentrant where libc's
`getopt` is not; `fmt_*`/`scan_*` have no libc equivalent without stdio.
One is a correctness question, not a size one: `path_gethome` (281 B)
looks up home directories without libc's `getpwnam`, so a dynamic build
misses NSS-provided users (LDAP etc.) — check before touching it.

### 5.4 Re-decide the `LINK_STATIC` mem-routine switch per libc -- DECIDED: keep it as is

`lib/byte.h:60` maps `byte_copy`/`byte_zero`/... to `memcpy`/`memset`/... when linking dynamically, and uses
the in-tree loops in `lib/byte/` and `lib/str/` when linking statically. The earlier note claimed that, on
musl, the libc routines are slightly smaller. **Re-measured with the whole `byte_*`/`str_*` family switched to the
libc routines for static builds of glibc, musl and dietlibc (`-DCMAKE_BUILD_TYPE=MinSizeRel -DLINK_STATIC=ON`,
`size` text, same tree before and after):**

```
                          in-tree loops (today)   libc routines
glibc static   text           1105876               1105876      (identical: the switch only differs for glibc)
musl static    text            234305                235217      +912
dietlibc static text           195191                195754      +563
stripped file size: 1242560 / 249816 / 208808 bytes, identical in all six builds (page alignment)
```

With libc routines the macros expand at every call site (`memcpy`/`memcmp`/`memset` get inlined or open-coded by
the compiler) and that costs more than a call to one small in-tree function; the libc `memcpy` the compiler
emits for struct copies is linked either way. So the in-tree loops are not "a second, slower copy that buys
nothing": they are smaller in `text` on all three libcs. **Decision: no change.** What the in-tree loops do cost
is speed on a libc with assembly `memcpy` (musl, glibc); nobody has measured a shell workload where that shows. If
one turns up, the answer is a per-function exception (`byte_copy` over `memcpy` only), not a per-libc switch.

### 5.5 Builtin set

`-DENABLE_ALL_BUILTINS=ON` costs 26 KB over the default set
(215232 vs 189312 stripped). The `EXTRA_BUILTINS` group (`cat`, `chmod`,
`ln`, `rm`, `mkdir`, `mktemp`, `uname`, ...) is what the container and
agent-sandbox pitch is built on, so it is not obviously droppable -- but
a documented "what does each builtin cost" table would let a distroless
image pick. Largest single builtins, text+data of the object:
`trap` 3987, `test` 3824, `printf` 3778, `expr` 3216, `set` 3120.

### 5.6 Not binary size, but on the same pitch: 262 KB of `.bss`

`sig_stack` 155648, `term_inbuf` 65535, `fdtable_table` 8200, `fd_list`
8192. It costs no file bytes and no RSS until touched, but a shell that
advertises itself for sandboxes should not reserve 155 KB of signal
stack. Worth a look after the above.

### Blockers found while measuring

- `BUGS: no-tree-print-option-is-a-noop` -- an existing size knob that
  does nothing.

### How to measure

```sh
cmake -S . -B /tmp/sz -DCMAKE_BUILD_TYPE=MinSizeRel -DDO_TESTS=OFF \
      -DBUILD_SHFORMAT=OFF <options>
cmake --build /tmp/sz -j8 && strip /tmp/sz/shish && stat -c%s /tmp/sz/shish
size -A /tmp/sz/shish          # per-section, spots .eh_frame-style bloat
nm --size-sort -S -td /tmp/sz/shish | tail -30
```

Always compare stripped sizes, and always re-run `tests/*.sh` with the
result -- `builtin-rmdir.sh` and `fixed.sh` already fail on `main`, so
match against a baseline rather than expecting green.

---

## Goal 6 (secondary) — regex engine `text/dfa`: what is left

Built as `text/dfa/` (header `text/dfa.h`, prefix `dfa_`; not `lib/dfa/`): Pike's NFA simulation
(`dfa_run.c`, one pass, bounded memory) and an explicit-stack backtracker (`dfa_bt.c`) for
patterns with back-references. `expr :`, `grep`, `sed` and `awk` run on it; tests are
`tests/builtin-{expr,grep,sed,awk}.sh`.

Open:

- **Lazy DFA** behind the same entry points (`dfa_test/prefix/search/submatch`) as a pure speed
  mode; not built (so `cbmap`/`hashmap` has no consumer for interning states). Only if a measured
  need appears. It needs a dev-only differential test (random patterns/subjects: DFA vs backtracker
  vs libc `regcomp`, including `(a*)*b`, `(a|aa)*b`, empty loops, 200 000-character subjects) and
  a test that forces the cache flush with a tiny limit.
- **More consumers**, only if wanted: `csplit` (BRE context lines), `ed`, `more` (`/re`), `pax -s`,
  `find -regex`. Not useful: `case`, `${x#pat}`, globbing (`path_fnmatch` is iterative and adequate).
- **Risks that still apply:**
  - backrefs make matching NP-hard: keep them off the NFA path and accept the exponential worst case;
  - the backtracker recurses once per group repetition: cap the depth or make single-atom repeats
    iterative so `\(a\)*` on a long line cannot exhaust the stack;
  - leftmost-longest holds for the overall match; sub-match choice deviates from the strict POSIX
    rule the way glibc does (musl/TRE is the stricter oracle);
  - `\| \+ \?` in BRE (`DFA_GNU`) are implementation-defined; decide with the first real user
    (≈30 lines in the lexer);
  - `sed`'s "empty regex = last regex" is run-time state: keep compiled regexes in the script's
    command array, not on the stack;
  - `w /dev/stdout` works, `r /dev/stdin` is deliberately left out (GNU-only).
- `expr` string comparison (`expr index` is fixed: first position of any character of CHARS).

---

## Goal 7 (secondary) — optional UTF-8 support (`WITH_UTF8`, off by default)

**Not started; this section is the plan.** shish is byte-oriented and
never calls `setlocale`, i.e. it is always in the POSIX locale, which is
what POSIX requires (bytes, C locale: Goal 6). This goal adds *character*
semantics for a UTF-8 locale (`LC_ALL=C.UTF-8`) as a compile-time option;
the default build stays C-locale-only and must not grow.

### What "UTF-8 as POSIX specifies it" means for a shell

In a locale whose charmap is UTF-8 a *character* is one UTF-8 sequence.
The places where POSIX makes the shell care:

- `${#param}` is a length in characters. (Measured 2026-09-19 with
  `x=é; echo ${#x}`: bash, zsh, ksh, busybox ash give 1 under
  `LC_ALL=C.UTF-8`; dash gives 2; shish gives 2.)
- Pattern matching — `?`, `[...]` members and ranges, and the stepping of
  `*` — in `case`, `${p#pat}`/`${p%pat}`/`##`/`%%`, and pathname expansion.
- IFS characters (field splitting, `"$*"`'s first-*character*-of-IFS
  separator, `read`). POSIX 2.6.5 defines IFS in terms of "the byte
  sequences that form the characters", so a multibyte IFS character is
  one separator, not several.
- Utilities' own character semantics: `wc -m`/`-L`, `expr :` and the regex
  utilities (Goal 6). **`printf` is not in this list**: POSIX.1-2024
  `printf` says `%c` prints the *first byte* of the argument and has no
  multibyte provision for `%s` precision either (bytes), so it stays
  bytewise (verified 2026-09-19); bash's character behaviour is an
  extension, not a model.
- The interactive line editor: cursor keys, backspace and redraw are per
  character, with display columns ≠ characters for wide/combining ones.
- Locale switching: `LC_ALL`, `LC_CTYPE`, `LANG` are shell variables whose
  assignment changes the shell's own behaviour.

What does **not** change: lexing, parsing, quoting, names, arithmetic,
redirections, job control. Every non-ASCII byte in UTF-8 is ≥ 0x80, so a
multibyte character never contains an ASCII shell metacharacter, and the
`\`-escape marker (one byte) protecting a lead byte is harmless (add a
test: `echo \é '*é*'`). Nothing in `lib/buffer`, `lib/stralloc`,
`lib/str`, `lib/byte` needs to change either: they are byte containers,
and `str_len()`/`byte_*` stay the right thing for sizes and allocation.

**So it is not only `${#x}` and range indexing**, but it is also not a
rewrite. Seven areas, listed by how invasive they are:

| # | Area | Sites | Invasiveness |
|---|---|---|---|
| A | language-visible expansions | `expand_param.c` (7 sites), `"$*"` separator | small |
| B | pattern matching core | `lib/path/path_fnmatch.c` (one function serves `case`, `%`/`#`, and our regex bracket sets) | moderate, one file |
| C | pathname expansion | `expand_glob.c` calls the **libc** `glob()` | policy decision (below) |
| D | IFS with non-ASCII characters | `expand_cat.c`, `expand_glob.c`, `builtin_read.c` | small, rare case |
| E | builtins | `builtin_printf.c`, `builtin_wc.c`, `builtin_expr.c`/`lib/dfa` | small each |
| F | line editor | `src/term/*` (906 lines, ~10 files) | **largest** |
| G | locale detection/switching | `sh_init.c`, `var_setsa.c` (the `RANDOM` special-case is the precedent) | small |

### Compile-time and runtime switches

- `src/features.h` already carries `WITH_*` toggles (`WITH_PARAM_RANGE`);
  add `WITH_UTF8` (default 0). CMake: `option(ENABLE_UTF8 ... OFF)` →
  `-DWITH_UTF8=1`; autotools: `--enable-utf8`.
- Runtime: even a `WITH_UTF8=1` binary is byte-wise unless the locale
  says otherwise. `mb_utf8` is 1 iff the effective `LC_CTYPE` (POSIX
  order: `LC_ALL`, else `LC_CTYPE`, else `LANG`) has a `.UTF-8`/`.utf8`
  charmap suffix, case-insensitive. shish decides this itself from the
  variables; it does **not** need libc's `setlocale` for its own logic,
  so it also works on dietlibc and mingw.
- Hooks: `sh_init()` (initial environment) and `var_setsa.c`/`var_unset.c`
  when the name is one of the three (same shape as the `RANDOM` case at
  `var_setsa.c:36`). **Risk:** temporary command-prefix assignments
  (`LC_ALL=C.UTF-8 wc -m`) may bypass `var_setsa`; verify with a test
  before trusting the hook.

### API (`lib/mb.h`, prefix `mb_`) — shaped like the byte code it replaces

Design rule: the non-UTF-8 code is the reference; the API is bent to fit
it, not the other way round. Two consequences:

1. Step functions return a **byte length to add/subtract**, so an
   existing byte loop keeps its shape: `i++` becomes
   `i += mb_clen(v + i, vlen - i)`, `i--` becomes `i -= mb_plen(v, i)`.
   In a byte build those are the constants `1`, and the compiler emits
   the same code as before.
2. There is **one** `#if WITH_UTF8` — in `lib/mb.h`, whose `#else` branch
   defines every function as a byte version (`static inline`, arguments
   evaluated once). Call sites carry no `#ifdef`; only code whose logic is
   structurally different in UTF-8 (bracket-member decoding in
   `path_fnmatch`, multibyte input assembly in the editor) gets an
   explicit `#if WITH_UTF8` block next to its byte version.

```c
#if WITH_UTF8
extern int mb_utf8;                                   /* 1 while the locale is UTF-8 */
void   mb_locale(void);                               /* re-read LC_ALL / LC_CTYPE / LANG */
size_t mb_clen(const char* s, size_t n);              /* bytes of the char at s; never 0 (n == 0 gives 1) */
size_t mb_plen(const char* s, size_t pos);            /* bytes of the char ending at s+pos; pos > 0 */
size_t mb_len (const char* s, size_t n);              /* characters in n bytes */
size_t mb_off (const char* s, size_t n, size_t chars);/* byte offset after `chars` characters, clamped to n */
size_t mb_cols(const char* s, size_t n);              /* display columns of n bytes (optional: needs width table) */
size_t mb_get (const char* s, size_t n, unsigned long* cp); /* decode: bytes consumed (>= 1), code point in *cp */
#else  /* byte versions: 1, 1, n, min(chars, n), n, (*cp = (unsigned char)*s, 1) */
#endif
```

`mb_clen` never returns 0 because the existing suffix loops visit
`i == vlen` (the empty suffix) and rely on stepping past it. Every
`mb_*` function starts with `if(!mb_utf8)` and returns the byte answer,
so a `WITH_UTF8=1` binary under `LC_ALL=C` behaves identically to the
default build. `mb_put` (encode) is deliberately left out until
something needs it (`printf '\u'` is not POSIX).

Policies (decide once, test once):

- Invalid, overlong, truncated-at-end-of-buffer, surrogate (D800-DFFF)
  or > U+10FFFF sequences: **each offending byte is one character**
  (width 1, code point = the byte value). Matches what bash does.
- A combining mark is a character of its own (`${#x}` counts it), as in
  bash; no grapheme clustering.
- Ordering (`ls`, glob results) stays byte order; for valid UTF-8 that
  equals code-point order, which is what `C.UTF-8` collation is (glibc
  documents this; verify). No `strcoll`.
- Non-ASCII `[:alpha:]`/`[:upper:]`… : **out of scope for the first pass**,
  classes match ASCII only (documented deviation). Optional later, only
  where libc can answer (`iswctype` after `setlocale`): glibc and musl.

### What the spec text says (POSIX.1-2024, read 2026-09-19)

| Statement | Where | Consequence here |
|---|---|---|
| `${#parameter}`: "the length in characters of the value of parameter" | 2.6.2 | `mb_len` in a UTF-8 locale; bytes in POSIX locale |
| IFS: "byte sequences that form the characters" | 2.6.5 | a multibyte IFS character is one delimiter |
| `"$*"`: fields joined by "the first character of IFS" | 2.5.2 | `mb_clen(ifs)` bytes, not `ifs[0]` |
| pattern matching: `?` one *character*, bracket members are characters | 2.14 | area B |
| POSIX locale = 256 single-byte characters, UTF-8 never mandated | 6.2 | a bytes-only shish is conformant; this whole goal is optional |
| `printf %c`: "first byte", no multibyte text for `%s` precision | printf | **no change** (see risks) |
| `wc -m`: number of characters; `-c` bytes | wc | area E |
| `LC_ALL` > `LC_CTYPE` > `LANG` precedence, null = unset | 8.2 | `mb_locale()` |

### `mb_*` algorithms (no libc, no `wchar_t`)

**What is a valid sequence** — the Unicode "well-formed UTF-8" table; the
whole validator fits in one `switch` and needs no `scan_utf8`:

| First byte | Length | 2nd byte | 3rd, 4th |
|---|---|---|---|
| 00-7F | 1 | — | — |
| C2-DF | 2 | 80-BF | — |
| E0 | 3 | **A0**-BF | 80-BF |
| E1-EC, EE-EF | 3 | 80-BF | 80-BF |
| ED | 3 | 80-**9F** (no surrogates) | 80-BF |
| F0 | 4 | **90**-BF (no overlong) | 80-BF ×2 |
| F1-F3 | 4 | 80-BF | 80-BF ×2 |
| F4 | 4 | 80-**8F** (≤ U+10FFFF) | 80-BF ×2 |
| 80-BF, C0, C1, F5-FF | invalid | — | — |

`mb_clen(s, n)`: if `!mb_utf8 || n == 0 || s[0] < 0x80` return 1; look up
the row; if fewer than `len` bytes remain or a continuation byte is out
of its range, return 1 (**each byte of an invalid or truncated sequence is
its own character**); else return `len`. Noncharacters (U+FFFE, U+FDD0..)
are valid, like glibc. This makes `scan_utf8` (which accepts surrogates
and > U+10FFFF, see below) unnecessary for lengths; it is only useful for
`mb_get`'s code-point value, and a 15-line shift/or over the validated
bytes does that too — so **no c-utils code has to be imported** unless
`fmt_utf8` (encoding) or the width table is wanted.

`mb_plen(s, pos)` (character ending at `s+pos`, `pos > 0`): `k = pos-1`;
while `k > 0 && pos-k < 4 && (s[k] & 0xC0) == 0x80` do `k--`; if
`mb_clen(s+k, pos-k) == pos-k` return `pos-k`, else 1. (`n = pos-k`
makes a sequence that would extend past `pos` count as truncated.)

`mb_len(s, n)`: `for(i = c = 0; i < n; c++) i += mb_clen(s+i, n-i);`
`mb_off(s, n, chars)`: same loop stopping after `chars` steps, clamped to `n`.
`mb_cols(s, n)`: sum of `wc_charwidth(cp)` per character, 1 for invalid
bytes, 0 for combining marks (M6 only).

**Property tests** (`tests/dev/mb-fuzz.c`, dev-only, ASan+UBSan, before
M2 lands): for random byte strings up to 64 bytes over the alphabet
{ASCII, 80, 8F, 9F, A0, BF, C0, C2, E0, E2, ED, F0, F4, F5, FF} (dense in
boundary cases): (1) walking forward with `mb_clen` and walking backward
with `mb_plen` give the **same boundaries**; (2) `mb_len` equals the
number of forward steps; (3) `mb_off(s, n, mb_len(s, n)) == n`; (4) each
step is ≥ 1 and never passes `n`; (5) for strings that are valid
UTF-8 (per `iconv -f UTF-8 -t UTF-8` on the host, or Python's decoder in
the driver script) `mb_len` equals the code-point count; (6) with
`mb_utf8 = 0` every function returns the byte answer.

**Locale detection** (`mb_locale()`): value = `LC_ALL` if set and non-empty,
else `LC_CTYPE` if set and non-empty, else `LANG` if set and non-empty,
else `""`. Format is `language[_territory][.codeset][@modifier]`: take
the text after the first `.` up to `@` or the end, remove `-` and `_`,
compare case-insensitively with `utf8`. Table: `C.UTF-8`, `en_US.utf8`,
`de_DE.UTF-8@euro` → on; `C`, `POSIX`, `""`, `en_US`, `en_US.ISO-8859-1`,
`en_US.UTF-16` → off. Cost: three `var_get` calls, done only when one of
the three variables changes (hook) and once at start-up. No `setlocale`.

### Per-site sketches (byte build = the existing code, unchanged)

`expand_param.c` — the removal loops already index bytes in `v[0..vlen]`;
only the step changes:

```c
/* %  (smallest suffix): today  for(i = vlen - 1; i >= 0; i--) */
for(i = vlen - mb_plen(v, vlen); ; i -= mb_plen(v, i)) {   /* mb_plen(v,0) not called: */
  if(path_fnmatch(sa.s, sa.len, v + i, vlen - i, 0) == 0) break;
  if(i == 0) { i = -1; break; }                            /* keeps the "no match" result */
}
/* %% (largest suffix): today  for(i = 0; i <= vlen; i++) */
for(i = 0; i <= vlen; i += mb_clen(v + i, vlen - i))       /* clen(…,0) == 1 steps past the end */
/* #  (smallest prefix): today  for(i = 1; i <= vlen; i++) */
for(i = mb_clen(v, vlen); i <= vlen; i += mb_clen(v + i, vlen - i))
/* ## (largest prefix):  today  for(i = vlen; i > 0; i--) */
for(i = vlen; i > 0; i -= mb_plen(v, i))
```

`S_STRLEN`: `expand_cat(lstr, fmt_ulong(lstr, mb_len(v, vlen)), …)`.
`S_RANGE` (`${v:off:len}`, characters like bash): `nc = mb_len(v, vlen);
r = limit(&r, 0, nc); o = mb_off(v, vlen, r.offset);
e = o + mb_off(v + o, vlen - o, r.length);` then `expand_cat(v + o, e - o, …)`.
`"$*"`: `sep = ifs[0] ? mb_clen(ifs, str_len(ifs)) : 0` and append
`sep` bytes (0 = no separator: the fix for `BUGS: star-empty-ifs-appends-nul`,
which must land first).

`path_fnmatch.c` — three changes, each behind the shared `mb_*` names:
`?` consumes `mb_clen(str, len)` bytes; the `*` backtrack advances the
subject start by `mb_clen` instead of 1 (so it never resumes inside a
character); a bracket member/range endpoint is read with `mb_get` from
the pattern and the subject character is read with `mb_get`, then
`lo <= cp && cp <= hi` (in a byte build the same comparison happens on
`unsigned char`, which is exactly today's code). Literal runs stay a
byte compare — correct because UTF-8 is a prefix code. `[[:class:]]`
uses the ASCII tables for cp < 0x80 and *never matches* above it (first
pass). An invalid byte in the subject matches only `?`, `*`, a negated
bracket or itself (as a single "character" with cp = byte value ≥ 0x80
outside any valid range, so it cannot be matched by `[à-ÿ]`: use cp =
0x110000 + byte for invalid bytes to keep it out of every real range).

`builtin_wc.c`: `-m` = `mb_len` of the buffer, carried across read chunks
(a character may straddle two reads: keep up to 3 bytes of carry, the
simplest correct form is counting non-continuation bytes for valid
input and falling back to `mb_len` per chunk otherwise; write the
straightforward carry version and test with 1-byte reads).

`builtin_read.c` / `expand_cat.c` / `expand_glob.c` (IFS, M6): today
IFS is a byte set queried with `str_chr`/`scan_charsetnskip`. With
non-ASCII IFS characters: build once per split a small array of
`(ptr, len)` characters from IFS; a position is a delimiter when
`mb_clen` bytes at that position equal one of them. All-ASCII IFS keeps
the byte set path (the common case, zero cost).

### Tests (`tests/utf8.sh`, written first — M0)

Run with the binary's own `LC_ALL=C.UTF-8` (set inside the script, as a
plain assignment and as a command prefix — the hook risk above) and with
`LC_ALL=C`. One assertion per line, "byte build" expectation in the
comment. Cases (`é` = C3 A9, `€` = E2 82 AC, `😀` = F0 9F 98 80, and
`\351` = a lone byte E9, invalid):
- `x=é; echo ${#x}` → 1 (UTF-8) / 2 (C); `x=€😀; echo ${#x}` → 2 / 7;
  `x=a\351b; echo ${#x}` → 3 / 3 (invalid byte = one character);
  `x=; echo ${#x}` → 0.
- `x=éa; echo ${x#?}` → `a` / `\251a` (C: strips one byte);
  `${x%?}`, `${x##*}`, `${x%%?}`, `${x#é}`, `${x#[é]}`, `${x#[à-ÿ]}`.
- `case é in ?) …` → match / no match; `case é in [é]`, `[a-z]` (no),
  `[à-ÿ]` (yes), `[!a]` (yes), `case €x in ?x)`, `case '*é' in \*?)`.
- `${x:0:1}`, `${x:1}`, `${x: -1}` on `éa€` (only if `WITH_PARAM_RANGE`).
- `set -- a b c; IFS=é; echo "$*"` → `aéb…` (separator = whole char);
  `IFS=€ read a b <<<` … splits at the 3-byte char; IFS=`é` and input
  containing byte A9 alone must not split.
- `echo \é '*é*'`, `echo "é"`, names `é=1` must still be an error
  (identifiers stay ASCII), redirect to a file called `é`.
- `printf '%c' é | od -c` → **one byte C3** in both builds (POSIX);
  `printf '%.1s' é` → C3.
- `wc -m` and `wc -c` on `é€😀` → 3 and 9 (UTF-8) / 9 and 9 (C).
- Line editor (M5): scripted through the pty helper in
  `tests/pty-run` — type `é`, press Backspace, expect an empty line; type
  `aé`, Left, Left, `X` → `Xaé`; a partial sequence written in two writes
  (C3, then A9) must produce one `é`.

### Sites (byte behaviour in the right column must stay exactly as today)

| File:line | Today | UTF-8 change | Est. lines |
|---|---|---|---|
| `expand/expand_param.c:265` `S_STRLEN` | `fmt_ulong(vlen)` | `mb_len(v, vlen)` | 2 |
| `:352` `S_RSSFX` (`%`) | `i` from `vlen-1` down by 1 | `i -= mb_plen(v, i)` | 4 |
| `:370` `S_RLSFX` (`%%`) | `i++` up to `vlen` | `i += mb_clen(v+i, vlen-i)` | 3 |
| `:388` `S_RSPFX` (`#`) | `i++` | same | 3 |
| `:409` `S_RLPFX` (`##`) | `i--` | `i -= mb_plen(v, i)` | 3 |
| `:423` `S_RANGE` (`${v:off:len}`, `WITH_PARAM_RANGE`) | byte offset/length | offsets are characters: `mb_off` twice | 6 |
| `expand_param.c:86-90`, `expand_glob.c:79-81` | `"$*"` joins with `ifs[0]` | `mb_clen(ifs, ifslen)` bytes | 4 |
| `lib/path/path_fnmatch.c` | `?`, `*` backtrack, bracket members/ranges by byte | `?` and star step by `mb_clen`; bracket members decoded with `mb_get` (`[éü]`, `[à-ÿ]` by code point); literal runs unchanged (byte compare is correct) | 40-60 |
| `expand/expand_glob.c` | libc `glob()` | see "libc" below | 5-10 |
| `expand_cat.c:10-16`, `expand_glob.c:42`, `builtin_read.c:122-141` | IFS as a *byte* set (`str_chr`, `scan_charsetnskip`) | only matters when IFS contains non-ASCII: add `mb_span`/`mb_cspan`. **P3**, known gap until done | 30 |
| `builtin_printf.c:308-345` | `%s` width/precision and `%c` in bytes | **none** — POSIX says bytes (see above) | 0 |
| `builtin_wc.c` | `-m` = bytes; `-L` = bytes | `-m`: `mb_len` (one per character; invalid byte = 1); `-L`: `mb_cols` | 20 |
| `builtin_expr.c`, `lib/dfa` | bytes | `length`/`index`; regex via `DFA_UTF8` (Goal 6) | later |
| `src/term/*` (below) | column = byte index | see below | 100-150 |
| `sh/sh_init.c`, `var/var_setsa.c`, `var/var_unset.c` | — | `mb_locale()` calls | 10 |
| `parse/*`, `source/*` error columns | byte column | cosmetic, **not planned** | 0 |
| `getopts` / `lib/shell/shell_getopt.c` | one byte per option | multibyte option letters: **not planned** | 0 |

**Line editor (F).** `term_pos` is at once the byte index into
`term_cmdline` and the number of columns, and `term_left(n)` emits `n`
backspaces. Least invasive fix: keep `term_pos` a **byte** offset and
make the cursor-motion primitives convert bytes to columns —
`term_left(bytes)`/`term_right(bytes)` compute `mb_cols(slice)` and emit
that many `\b`/`CSI D`. Callers that already work in bytes stay as they
are. Only the "one character" operations change: `term_backspace`
(remove `mb_plen` bytes), `term_delete` (`mb_clen`), the `C`/`D` arrow
handling in `term_ansi.c` (count characters via `mb_off`),
`term_insertc` (takes a whole character: `term_read.c:188-190` must
buffer a partial sequence until complete), and `term_complete.c`
(common-prefix truncation at a character boundary; per-char insertion).
Without this, a UTF-8 build corrupts the command line on the first
backspace over a multibyte character, so **M5 gates enabling `WITH_UTF8`
for interactive use**. There is no prompt-width tracking today, so
nothing to change in `prompt/`.

**libc (C).** Pathname expansion is the platform's `glob()`
(`expand_glob.c:5-19`; `lib/unix/glob.c` is Windows-only), and glibc/musl
`glob`/`fnmatch` follow the locale set with `setlocale`, which shish
never calls. Options: (a) leave it — `?` in a *pathname* pattern stays
bytewise, everything else is right (documented gap); (b) under
`HAVE_SETLOCALE`, call `setlocale(LC_CTYPE, "")` when `mb_utf8` turns on
so libc agrees (glibc, musl); dietlibc/mingw stay at (a). Start with (a),
add (b) as its own step. The internal `glob` planned in Goal 5.3
(`USE_LIBC_GLOB=OFF`) removes the problem for static builds without
`setlocale`: it matches with `path_fnmatch`, which M3 makes UTF-8-aware.

### What was found in the existing UTF-8 code (looked at only after the design above)

shish's own `lib/utf8/` (8 files, `u8len`, `u8towc`, `wctou8`, …) is
**unused** anywhere in `src/` or `lib/` — flagged, not to be removed
unasked. It is not a base for this: `u8len` trusts the lead byte and
never checks continuation bytes, `u8towc` does not reject overlong forms,
and the API is built on `wchar_t`, which is 16-bit on mingw (a supported
target), so code points above U+FFFF do not fit.

`../c-utils/lib` (tried `utf8/`, `ucs/`, `scan/scan_utf8*.c`,
`fmt/fmt_utf8.c`):

| Function | Verdict |
|---|---|
| `scan_utf8(in, len, &cp)` (85 lines) | **use.** Decodes strictly: rejects overlong, stray continuation, truncated, `0xFE`/`0xFF`. Tested 2026-09-19: **accepts** surrogates (`ED A0 80` → U+D800), values above U+10FFFF (`F4 90 80 80`) and 5-byte forms (`F8 88 80 80 80`), so `mb_get` must add `cp <= 0x10FFFF` and a surrogate check (2 lines). Returns 0 on invalid → policy above (one byte, one character). Independent of `wchar_t`. |
| `scan_utf8_sem` | no: also rejects the noncharacters `U+FFFE/FFFF`, `U+FDD0-FDEF`, which are valid scalar values glibc accepts |
| `fmt_utf8(dest, cp)` (35 lines) | keep in reserve for `mb_put` |
| `wc_charwidth(wchar_t)` (61 lines, Kuhn's wcwidth) | **optional**, for `mb_cols`. Linear scan over ~150 range pairs (fine for an editor), Unicode 5-era tables (newer wide emoji are missing), `wchar_t` argument (change to `uint32`). ≈1.5 KB of tables. |
| `u8b_len` | no: counts an invalid byte as **6** (HTML-entity width), shell needs 1 |
| `u8_len` | no: returns 0 for both NUL and invalid, trusts the lead byte |
| `u8b_chrs`, `u8b_rchrs`, `u8*_diff`, `u8s_*` | not needed (search/compare helpers) |
| `wcs_to_u8s`, `u8_to_wc`, … | no: `wchar_t` |
| `ucs/*` (Latin-1 conversion) | not relevant |

Plan (revised after writing the algorithms above): `mb_clen` embeds the
Unicode well-formedness table itself (one `switch`, no surrogate/range
gap to patch), so `scan_utf8.c` is **not needed** for lengths; copy it
only if `mb_get` should reuse its decoder. `fmt_utf8.c` and
`wc_charwidth.c` are copied later, only if `mb_put`/`mb_cols` are built.

### Size estimate (estimates, nothing written)

| Piece | Lines | Code, `WITH_UTF8=1` |
|---|---|---|
| `lib/mb.h` + `lib/mb/` (`mb_locale`, `mb_clen`, `mb_plen`, `mb_len`, `mb_off`, `mb_get`, `mb_cols`) | ≈250 | ≈2-3 KB |
| copied later, optional: `fmt_utf8` 35, width table 61 (`scan_utf8` no longer needed) | ≈100 | ≈0.3 KB + ≈1.5 KB tables |
| call-site edits in A, B, D, E, G | ≈150-200 | ≈1-1.5 KB |
| editor (F) | ≈100-150 | ≈1 KB |
| tests (`tests/utf8.sh`) | ≈200 | — |
| **Total** | **≈500-650 lines** | **≈4-6 KB (+1.5 KB with width tables)** |

**Default build (`WITH_UTF8=0`): 0 bytes** — that is a requirement, not a
hope: compare the stripped `shish` size before/after each milestone
(Goal 5's tooling) and treat any growth beyond a few bytes of noise as a
bug in the `#else` branch.

### Order of work (each step its own change with test, `fixes/NN` where it fixes something, and this file updated)

- **M0** — `tests/utf8.sh` with the oracle table (bash's answers; dash
  as the byte reference): it must pass in *both* builds, asserting the
  C-locale answers when `WITH_UTF8=0` **even with `LC_ALL=C.UTF-8`
  exported** (that is the "tiny C-only binary" contract).
- **M1** — `lib/mb` + `mb_locale()` + hooks (G). `${#x}` alone flips.
- **M2** — area A: `${#}`, the four removal forms, `${v:off:len}`,
  `"$*"` separator.
- **M3** — area B: `path_fnmatch` (also fixes `case` and `%`/`#`
  pattern semantics in one place).
- **M4** — area E: `wc` (`printf` stays bytes), then C(b) `setlocale` if wanted.
- **M5** — area F: line editor. Gate for interactive `WITH_UTF8`.
- **M6** — area D (non-ASCII IFS), width table for wide characters.
- **M7** — Goal 6's `DFA_UTF8`: character sets as code-point ranges,
  compiled to UTF-8 byte-sequence automata so the DFA stays byte-based
  (the 256-bit bracket sets in Goal 6 cover the ASCII part only).

### Risks and open questions

- Character classes for non-ASCII stay ASCII-only in the first pass;
  say so in `--help`/docs, or it will be reported as a bug.
- `LC_ALL=C.UTF-8` as a command-prefix assignment: see the hook risk above.
- `${v:off:len}` is a non-POSIX extension already on by default
  (`WITH_PARAM_RANGE`); its UTF-8 behaviour follows bash (characters).
- Invalid input: nothing may crash or loop — fuzz the `mb_*` functions
  with random bytes under ASan+UBSan (MAIN QUEST gate) before M2 lands.
- `printf` `%c`/`%.Ns` stay bytes (POSIX.1-2024); a later "bash
  compatibility" request to make them characters is a scope change, not a bug.
- `"$*"` with an empty IFS is broken independently of UTF-8
  (`BUGS: star-empty-ifs-appends-nul`); fix it before touching that
  line, or M2's `mb_clen(ifs, …)` would return 1 for the NUL terminator.

---

## Goal 8 (secondary) — `shfuzz`: a structure-aware shell fuzzer built on `union node`

**Not started; this section is the plan.** Generate random but *valid*
ASTs (`src/tree.h`) from entropy or a key, serialize them with
`tree_cat()`, and feed the text to the parser/evaluator. Useful as a
test (round-trip and sanitizer oracles) and as a standalone utility.
Dev tool: built only with a `BUILD_SHFUZZ` option, **never linked into
`shish`** (the constraint table alone would cost several KB).

### What `tree_cat.c` shows (checked 2026-09-19 by grep and by running `shformat`)

`tree_cat()` is the ground truth for which parts of `union node` matter
to a serializer — but it is lossy, and the loss is exactly the set of
members it never reads:

| Read by `tree_cat` | Never read |
|---|---|
| child slots (`args vars rdir cmds pats word list test cmd0 cmd1 body left right node cond ontrue onfalse tree`), `nargstr.flag & S_TABLE`, `nargparam.flag` (`S_STRLEN S_VAR S_SPECIAL S_ARITH S_NULL`, `S_RANGE` under `WITH_PARAM_RANGE`) + `numb` + `name`, `nargcmd.flag & S_BQUOTE`, `nredir.flag` (`R_IN R_OUT R_APPEND R_DUP R_HERE R_STRIP R_CLOBBER`) + `fdes`, `narithnum.base` ∈ {8,10,16} + `num`, arithmetic ids, `bgnd` of every statement kind (`tree_isbgnd()`), `nfor.has_in` | `nredir.data`, `nredir.fd`, `npipe.ncmd`, all `loc`, `narg.flag` (the `X_*` expansion bits) |

One real bug is left from the right-hand column (`BUGS:
tree-cat-mangles-here-documents`); the `bgnd` and `has_in` ones are fixed
(`fixes/327`, `fixes/328`; `for x in; do …` used to print as `for x; do …`,
which changes the program). A generator restricted to what `tree_cat`
reads would never exercise here-doc data, although `eval` uses it — so
fix `tree_cat` first, or the round-trip oracle fails on every tree that
touches it.

### The constraint table: `tree_spec[]`, one row per `enum kind`

`enum kind` has 63 values (`N_SIMPLECMD` … `A_VBITOR`), and
`tree_nodesizes[]` (`src/tree/tree_nodesizes.c`) is already a per-kind
table, so the shape is established. Use C99 designated initializers
(`[N_REDIR] = {…}`) — `tree_nodesizes[]` is positional and would silently
shift if the enum were reordered. A set of allowed kinds fits a
`uint64_t` mask (bit `1ull << kind`, one spare bit).

```c
struct tree_member {               /* one *slot* of one node struct */
  uint16_t offset;                 /* offsetof(struct nredir, flag) */
  uint8_t  role;                   /* ONE OPT LIST | INT FLAGS ENUM | STR | COUNT */
  uint8_t  sep;                    /* LIST: what ->next means here (';' '|' ' ' ',' ...) */
  uint16_t min, max;               /* LIST length */
  uint64_t kinds;                  /* child roles: allowed kinds, bit per enum kind */
  int64_t  lo, hi;                 /* INT/ENUM: inclusive bounds (named TREE_MAX_* constants) */
  const struct tree_group* groups; /* FLAGS: independent sub-fields, e.g. S_TABLE = {0, S_DQUOTED, S_SQUOTED} */
  uint8_t  pred;                   /* STR: TS_NAME TS_SQ TS_DQ TS_WORD ... (see below) */
};
struct tree_spec {
  uint16_t size;                   /* == tree_nodesizes[kind]; static assert */
  uint8_t  nmember, min_depth;     /* min_depth: levels needed to finish, so generation terminates */
  struct tree_member m[TREE_MAXM];
  void   (*fixup)(union node*);    /* rare cross-member rules, see below */
};
const struct tree_spec tree_spec[A_VBITOR + 1] = {
  [N_SIMPLECMD] = {…, { LIST(ncmd, args, ' ', 0, 8, K(N_ARG)),
                        LIST(ncmd, vars, ' ', 0, 4, K(N_ASSIGN)),
                        LIST(ncmd, rdir, ' ', 0, 3, K(N_REDIR)) } },
  [N_REDIR]     = {…, { FLAGS(nredir, flag, R_GROUPS), INT(nredir, fdes, 0, TREE_MAX_FD),
                        LIST(nredir, word, '\0', 1, 1, K(N_ARG)) } },
  …
};
```

**`next` is not a member of the table.** Every one of the 22 `n*` structs
has it (common prefix `id`, a 1-bit field, `next`), so "has `next`" says
nothing. Whether a node sits in a chain is decided by the *slot that
points at it*:

| Role | Meaning | Validator rule |
|---|---|---|
| `ONE` | exactly one node | `->next == NULL` |
| `OPT` | zero or one | `NULL`, or a node with `->next == NULL` |
| `LIST` | chain, `min`..`max` long, separator tag `sep` | each element's kind ∈ `kinds`; last `next` is `NULL` |

The tree root is a pseudo-slot of role `LIST` (the top-level command
sequence) and is the generator's entry point. `sep` records that `next`
means `;` in a compound list, `|` in `npipe.cmds`, a space in
`ncmd.args`, `|` between case patterns, `,` in `A_PAREN`.

Initial slot roles, read from `tree_cat.c` (to be confirmed by method C):

| Slots | Serialized with | Role |
|---|---|---|
| `ncmd.args/vars/rdir`, `nfor.args/cmds`, `nloop.test/cmds`, `ngrp.cmds`, `nlist.cmds`, `ncase.list` (of `N_CASENODE`), `ncasenode.pats/cmds`, `nif.test/cmd0/cmd1`, `nargcmd.list`, `nargarith.tree`, `A_PAREN.tree`, `nredir.word` | `tree_catlist` / manual loop | `LIST` |
| `npipe.cmds`, `narg.list` (of `N_ARGSTR/ARGPARAM/ARGCMD/ARGARITH`) | manual loop | `LIST` |
| `ncase.word`, `nargparam.word`, `nfunc.body`, `A_TERNARY` slots, binary `left`/`right`, unary `node` | `tree_cat` | `ONE` |
| `nandor.left/right`, `nnot` | `tree_catlist` | **unclear** — the parser probably builds a single node; a chain given to a `tree_cat`-single slot silently loses its tail |

Rules that are not per-slot:

- The 1-bit field is called `bgnd` in 9 structs (`ncmd npipe nandor ngrp
  nfor ncase nif nloop nlist`), `dummy` in 7 (`nnot ncasenode nfunc
  narithnum/unary/binary/ternary`), and the 6 word-level structs (`narg
  nredir nargstr nargparam nargcmd nargarith`) have a full `unsigned flag`
  instead; the validator requires `dummy` bits to be 0.
- `nif.cmd1` holding exactly one `N_IF` prints as `elif`; `else if … fi`
  parses to the same shape, so both are the same tree.
- `nfunc.next` is typed `struct nfunc*`; treated as the same link.
- Whole-tree invariants: acyclic, and no node reachable twice
  (`tree_free` would double-free).
- Rare cross-member rules go in the per-kind `fixup` hook, only for:
  `npipe.ncmd == length(cmds)`; `nargparam` uses `numb` when
  `S_SPECIAL == S_ARG`, else `name`; an `R_HERE` redirection needs `data`.
- Strings are constrained by **predicates that already exist**
  (`parse_isname`, `parse_isesc`, `parse_isdesc`, `var_valid`) — the
  table stores a predicate id, the generator calls the predicate.
  Examples: a `'…'` string cannot contain `'`; `nfor.varn`, `nfunc.name`
  and `nargparam.name` must be valid names and not reserved words.
- Integers need **named, asserted bounds** (`TREE_MAX_FD` for
  `nredir.fdes`, one for `numb`, `num`/`base`), enforced in the parser
  as well; today they are plain `int`/`long`/`int64` and it has not been
  checked what `parse`/`eval` really tolerate.
- Not in the table: evaluation safety (the fuzzer must not generate
  `rm -rf`); that is generator policy, see below.

The same table drives `tree_validate(node)` (first out-of-spec member,
with a path). Uses: the generator's postcondition; a debug-build check
after every `parse()` — which enforces "integers never exceed what
parse/eval/expand expect" and finds either a wrong table or a parser
emitting out-of-spec trees. Later, `tree_free`/`tree_copy`/a
`tree_equal` could be made table-driven; not part of this goal.

### How the table gets filled: three derivations, used together

- **A. Static extraction from `tree_cat.c`.** Parse the file with clang
  (`-ast-dump=json` or `clang-query`); per `case` label record member
  accesses, constants in `& S_…`/`& R_…` tests, `switch` labels (the `base`
  cases), null checks (⇒ optional), `for(… n = n->next)` loops and
  `tree_catlist` vs `tree_cat` calls (⇒ `LIST` vs `ONE`), array sizes as
  domains (`vsubst_types[(flag & S_VAR) >> 8]` ⇒ 0..7). Emit an X-macro
  header. Mechanical and regenerable; needs clang (dev-only). Traps:
  `goto again`, the `N_NOT → N_AND` fallthrough, `#if WITH_PARAM_RANGE`,
  and accesses through an aliasing member (`node->ncmd.rdir` on `N_IF`
  relies on identical offsets — emit `offsetof` equality checks).
  Decides **which bits matter**.
- **B. Black-box sensitivity probing.** Compile against the real structs;
  per kind build a canonical valid node, flip every scalar bit and see
  whether the `tree_cat` output changes; sweep values for crashes; try
  every kind in every pointer slot for the allowed-child matrix
  (≈63 × 4 probes); cross-check against `tree_free`/`tree_copy`. Needs no
  C parsing and tests real behaviour, but cannot tell a domain violation
  from a bug the parser can reach, and is too permissive where `tree_cat`
  tolerates what `eval` would not. Used as a **CI check** that the table
  still matches reality.
- **C. Parser-observed profile.** A small tree walker run over every
  script we have (`tests/*.sh`, `tests/posix/*.tst`, `tests/yash`);
  record per (parent kind, slot) the child kinds, chain lengths and the
  observed value ranges of scalars. Stays within what `parse`/`eval`/
  `expand` handle in practice — the stated requirement — but only as
  complete as the corpus. Decides **domains and bounds** and settles the
  unclear `ONE`/`LIST` slots.

### What the generator is (definition, 2026-09-19)

A deterministic function `decode(bits) → AST`, i.e. a *structure-aware
test-case generator* with a fuzzing driver on top (prior art: Hypothesis'
choice sequences and Zest/JQF generators-as-parsers-of-bytes, LLVM's
`FuzzedDataProvider`, `libprotobuf-mutator`; for enumeration SmallCheck
and Korat). Required properties:

- **Total:** every bitstring decodes to a valid AST, nothing is rejected.
  Budgets and "default choice when the bits run out" deliver this.
- **Deterministic:** same bits → same AST → same script text. This is a
  property of *generation only*: executing the script is not
  deterministic (time, pids, `$RANDOM`, jobs, signals), so "same key,
  same behaviour" does **not** hold in evaluate mode.
- **Surjective** onto the valid ASTs within the budgets.
- **Many-to-one, and that is fine:** trailing unused bits, leading zeros
  and default choices collapse. "Every bit gives a different AST" is not
  true of a stream decoder.

Two designs share the same `tree_spec[]`; build the first:

| | Stream decoder (**first**) | Enumerative unranking (later) |
|---|---|---|
| Mapping | bits → AST, many-to-one | integer *n* → the *n*-th AST by size, bijective |
| For | random and coverage-guided fuzzing, mutation, **shrinking** a failing case by shortening/simplifying the stream | exhaustive small scope: "no crash for every AST up to size *k* over this word pool" |
| Weak | no coverage guarantee | no bit locality, so it does not mutate well |

"Counting 1, 2, 3, … generates every script" is only true of the
bijective design, and only in principle: binary counting changes the low
bits fastest, so a stream decoder fed 1, 2, 3, … varies only its last
~20 choices (early structure stays on defaults), and the space explodes
(8 words, commands of 1-3 words, scripts of 1-3 commands is already
≈2·10⁸ scripts before any nesting). Enumeration is only useful at small
size, as a guarantee, not as a way to find bugs in big scripts.

**ASTs are a quotient of scripts, not all scripts.** Whitespace,
comments, quoting variants and line continuations collapse into one AST,
and scripts that fail to parse are not ASTs. So valid-AST generation
never reaches the lexer's odd paths or the parser's rejection and
error-recovery paths — where memory-safety bugs often live — and `tree_cat`
emits one canonical style, so the round trip only tests that slice.
Two additions, both in scope:

- a **second serializer with randomized style** (extra whitespace,
  alternative-but-equivalent quoting, comments, `\`-newline
  continuations); the round-trip oracle still holds because the tree must
  not change;
- shfuzz output as **seed corpus** for byte-level fuzzing (AFL/libFuzzer)
  of the parser, which covers the invalid-input side.

### Words (strings) are synthesized, not enumerated

Strings dominate the search space (unbounded length × 256 symbols) and
the parser treats characters by class, so collapse them — deterministically,
from the same stream:

- **Role-typed pools:** name role → a small pool or `[a-z_][a-z0-9_]{0,k}`;
  numbers `0 1 -1 …`; glob patterns `* a* [a-c] ?`; fds; here-doc
  delimiters. The role comes from the table's string predicate.
- **Reduced alphabet:** one representative per character class the lexer
  and expander distinguish (letter, digit, `_`, space, tab, newline, `/`,
  `.`, `-`, `=`, `$`, `\`, `'`, `"`, `` ` ``, `*`, `?`, `[`, `]`, `!`, `~`,
  `#`, `%`, `{`, `}`, `|`, `&`, `;`, a non-ASCII byte).
- **Escape hatch:** one choice bit occasionally draws raw bytes, so rare
  bytes are not lost entirely.
- **Typed symbol pools:** variables, functions and files created earlier
  in the generated script are reused by later words; evaluation coverage
  is much better than with random names. This makes the generator
  slightly context-sensitive, kept as a small side table.

### Generator mechanics

- Input is a **byte stream**: every choice takes bits from it (`take(n)`);
  when it runs out, choices default to the minimal option. A key seeds a
  PRNG (splitmix64) that expands into that stream, so "from entropy or a
  key" is the same code, and libFuzzer/AFL can drive it with their own
  mutation of the bytes.
- Budgets: max depth, max list length per slot (`max` in the table),
  max string length, total node count; `min_depth` guarantees every
  recursion can be closed.
- Two modes: **parse-only** (safe by construction — *the first release*)
  and **evaluate**, which is a project of its own: word/command
  whitelist, scratch directory, `ulimit`s, timeouts for generated
  `while true` loops, no `/dev`-touching redirections, and normalisation
  of nondeterminism before any comparison. Evaluate mode stays **out of
  the first release**.

### Oracles

1. **Round trip:** `tree_cat(T)` → `parse` → `tree_cat` must reproduce
   the string, and the trees must match under a new `tree_equal` that
   ignores `loc`. Deviations are serializer or parser bugs (`tree_cat`
   fixes first, see above).
2. `tree_validate` on every parser output (debug builds).
3. ASan+UBSan build must run clean (MAIN QUEST gate); crash-only mode
   for long runs.
4. Optional differential run of the evaluated result against `dash`/
   `bash`/`yash` in a sandbox, for the POSIX-only subset the generator
   emits (evaluate mode only; noisy — bugs and bash-isms on both sides).

Without a reference shell the oracles find crashes, hangs, sanitizer
errors and round-trip mismatches, **not wrong behaviour**; say so in the
tool's documentation.

### Order of work (each step its own change with test and this file updated)

0. Fix the three `tree-cat-*` bugs in `BUGS` (regression cases in
   `tests/fixed.sh`, `fixes/NN`), or the round trip cannot be the oracle.
1. `TREE_MAX_*` constants and their parser-side asserts; write the tree
   walker (also needed by C) and `tree_equal`.
2. Method C profile over the corpus → first hand-written `tree_spec[]`
   for a subset (simple commands, pipelines, lists, words).
3. `tree_validate` + debug-build hook after `parse()`; every violation
   found is either a table fix or a `BUGS` entry.
4. Stream-decoder generator for that subset with word synthesis,
   round-trip test in `tests/`, ASan run. **This is the MVP and the
   go/no-go point:** ship `shfuzz` as a sibling of `shparse2ast`
   (`BUILD_SHFUZZ`, off by default, not installed, not linked into
   `shish`) only once it has found bugs.
5. Methods A and B: regenerate/verify the table; extend to compound
   commands, redirections, parameter expansion, arithmetic.
6. Randomized-style serializer (lexical noise) and shfuzz output as a
   seed corpus for AFL/libFuzzer on the parser; shrinking by stream
   reduction; standalone command line (`shfuzz KEY`, `shfuzz -n COUNT`).
7. Later, separately: enumerative unranking for small-scope exhaustive
   runs; evaluate mode with its sandbox.

### Size estimate (estimates, nothing written; calibration: `tree_free.c`
213 lines and `tree_copy.c` 194 lines are per-kind switches, `tree_cat.c` 613)

| Piece | Lines |
|---|---|
| `tree_spec[]` (63 rows, macros for the shared layouts) | ≈250-350 |
| `tree_validate` | ≈100-150 |
| `tree_equal` + walker | ≈150-200 |
| generator (byte-stream decoder, budgets, predicates, word synthesis) | ≈400-600 |
| randomized-style serializer (step 6) | ≈150-250 |
| method A extractor (clang script) / B prober / C profiler | ≈150 / ≈200 / ≈120 |
| harness + CLI + tests | ≈300 |

### Risks and open questions

- Parse-time state that the AST does not carry: aliases, `loc`/`$LINENO`,
  reserved-word context — the round trip must run with aliases off and
  compare without `loc`.
- Quoting is the hard part of the string domains: `nargstr` content
  depends on the enclosing quote state (`parse_isesc`/`parse_isdesc`); a
  wrong predicate makes every round trip fail for the wrong reason.
- Keep the table dev-only; if a debug-build `tree_validate` hook is
  wanted in `shish`, it must stay behind `DEBUG_*` like the other
  instrumentation.
- Method C is bounded by the corpus; legal-but-unseen combinations need
  the grammar in POSIX 2.10 as a cross-check.
- **Maintenance coupling:** every change to `tree.h` touches the table;
  static asserts (`sizeof`, `offsetof`, row count) and method A's
  regeneration reduce this but do not remove it. If the tool rots, delete
  it rather than let a stale table produce wrong reports.
- Word synthesis collapses the string space by design; the raw-byte
  escape hatch keeps some weird-byte coverage, but string-level bugs
  (glob, `printf` formats, IFS) still want their own byte-level fuzzing.
- Value: mainly for shell implementers and for CI (reproducible key →
  bug report, "no crash for all ASTs ≤ *k*"); end users get little, hence
  a build option and no installation by default.

---

## Goal 9 (secondary) — tab-completion: context-aware, and extensible through `complete`

**Not started beyond the first slice; this section is the plan.** (The
file is `src/term/term_complete.c`, 363 lines; there is no
`term_completion.c`.) Today TAB completes *file names*, and, since the
first-word change, also reserved words that begin a construct, builtins
and function names at the first word of a command (`tests/term-complete.sh`).
This goal turns that into a completion engine with three layers:
**what kind of word is under the cursor** (context), **where candidates
come from** (sources), and **who may add sources** (`complete`/`compgen`/
`compopt`, bash-compatible subset).

### What exists and what is missing (probed 2026-09-20 with a pty driver)

| Input, TAB at `|` | Today | Wanted |
|---|---|---|
| `ls "my fi|` (file `my file`) | nothing | `ls "my file"` — quote-aware word split |
| `ls my\ fi|` | nothing | `ls my\ file ` — escape-aware |
| `cd |` | lists files and dirs | directories only |
| `echo $HO|` | nothing | `$HOME` (variable names) |
| `pytho|` (first word) | nothing | executables from `$PATH` |
| `don|`, `fi|`, `esac|`, `els|`, `the|` | nothing | the closer/continuer, **when valid** |
| `for x i|` / `case x i|` | nothing | `in` |
| `if true; th|` | nothing | `then` |
| `echo x > a|` | files, dirs unmarked | files; dirs get `/` in the list |
| `kill -|`, `trap '' |`, `fg %|`, `unset V|` | file names | signals / signals / jobs / variables |
| `\` in a filename, `~user/`, `$VAR/x` | partly (`~/` only) | all three |
| a candidate list of 3000 entries | prints all | asks first ("Display all N?") |

Facts about the code that shape the design (all checked 2026-09-20):

- TAB is one `case '\t'` in `term_read()` (`term_read.c:184`). The editor
  holds only the **current line** (`term_cmdline`); earlier lines of a
  multi-line command were already handed to the parser, so "am I inside an
  `if`?" is not visible to the editor today.
- `struct parser` is a local object created by `parse_init()`, not a
  singleton, so a second parse over a string is possible; but
  `parse_error()` prints through `sh_msg()`, so a dry run needs a silent mode.
- Running shell code from inside the editor has a precedent: `prompt_show()`
  expands PS1 (which may contain command substitution) while `term_read()`
  is active.
- Data already there: `builtin_table[]`, `functions` (linked
  `union node`, name in `nfunc.name`), `parse_aliases`, the variable table
  (`var_search`/`var_hsearch`), `job_list`, `exec_hashtbl` (only the *hashed*
  commands, not all of `$PATH`), `sig_name()`, `expand_tilde_lookup()`.
- shish has **no arrays** (`a=(1 2)` is a syntax error), so bash's
  `COMP_WORDS`/`COMPREPLY` arrays need an equivalent (see Phase 5).
- `term_complete.c` is compiled out on `WINDOWS_NATIVE`; no `opendir`
  fallback there.
- Candidates are `str_dup`'d one by one and de-duplicated by a linear scan;
  fine for hundreds, quadratic for a `$PATH` with 3000 commands.

### Design

```
 TAB ─▶ compl_words(line, pos)   split with quotes/escapes, find the word under
                                  the cursor, keep its raw and unquoted form
      ─▶ compl_context(words, pending)   classify → CTX_COMMAND / CTX_ARG(cmd, n) /
                                  CTX_REDIR / CTX_VAR / CTX_TILDE / CTX_JOB /
                                  CTX_IN / CTX_KEYWORD_POS(state)
      ─▶ compl_spec(ctx)         pick a spec: user-registered (`complete`) first,
                                  else a built-in default for that command/context
      ─▶ compl_generate(spec, cur, &set)   run the sources, filter by prefix
      ─▶ compl_apply(set)        common prefix, suffix, list, escape on insert
```

- **One candidate set** (`struct compl`): a `stralloc` arena plus an offset
  array (no per-name allocation), sorted once, de-duplicated after the
  sort; flags per set — `nospace`, `filenames` (quote/escape on insert,
  `/` for directories), `dirs_only`. The name/source of each candidate is not
  kept; kind is only needed for the trailing character, which a set-wide
  flag plus one `stat()` for the unique-match case already gives.
- **`compl_generate()` is a pure function** of `(line, pos)` that fills a set
  and prints nothing: TAB calls it, `compgen` calls it, and tests call it
  without a pty (Phase 0 adds `compgen --line 'LINE' [POS]`, a non-bash
  debugging switch that prints one candidate per line).
- **Defaults are specs in the same format as user specs**: the built-in
  knowledge ("`cd` takes directories", "`kill` takes signals or jobs") is a
  compiled-in table of `struct compspec` rows, so Phase 5's registry is
  "the same table, with more rows added at run time" and a user can override
  any default with `complete -r cd; complete -A file cd`.

`struct compspec` (Phase 3 defines it, Phase 5 makes it user-visible):

```c
struct compspec {
  const char* name;      /* command this applies to; NULL = default (-D), "" = empty line (-E) */
  unsigned actions;      /* CA_ALIAS | CA_BUILTIN | CA_COMMAND | CA_DIR | CA_FILE | CA_FUNCTION |
                            CA_EXPORT | CA_JOB | CA_KEYWORD | CA_SIGNAL | CA_USER | CA_VARIABLE |
                            CA_SETOPT | CA_HELPTOPIC | CA_RUNNING | CA_STOPPED ... */
  const char* words;     /* -W: word list, expanded at completion time, split with $IFS */
  const char* func;      /* -F: function name */
  const char* cmd;       /* -C: command (needs fork: not on WASI) */
  const char* filter;    /* -X: glob; matching candidates are removed */
  const char* prefix;    /* -P */
  const char* suffix;    /* -S */
  unsigned opts;         /* -o: default dirnames filenames noquote nosort nospace plusdirs */
};
```

### Phase 0 — refactor and test harness (no behaviour change)

1. Split `term_complete.c` along the diagram above, one function per file as
   in the rest of `src/`: `compl_words.c`, `compl_context.c`,
   `compl_src_file.c`, `compl_src_names.c` (keywords/builtins/functions/
   aliases/variables/jobs/signals), `compl_set.c` (arena, sort, dedupe,
   common prefix), `compl_apply.c` (insert/list/redraw). `term_complete()`
   stays the public entry and becomes ~15 lines. Keep `WINDOWS_NATIVE`
   compiled out as today.
2. Introduce `struct compl` and make the existing file/keyword/builtin/
   function sources fill it. The de-dup becomes sort-then-uniq.
3. Add `compgen --line 'LINE' [POS]` (dev switch) and move the existing
   `tests/term-complete.sh` assertions to it; keep **a few** pty cases for
   the editor glue only (insertion, redraw, list), from a shared
   `tests/pty-drive.py` instead of the copy embedded in the test today.
   Gate: `tests/term-complete.sh` result unchanged before/after.

### Phase 1 — every reserved word at command position (the requested change)

Today the first-word list is `case do elif else for function if then until while`.
Add the rest so any keyword can be completed:

| Word | Offered at | Note |
|---|---|---|
| `done` `esac` `fi` `}` `)` | command position | closers; Phase 4 narrows to "only when one is open" |
| `else` `elif` `then` `do` | command position | continuers; Phase 4 narrows |
| `in` | **not** a command position: the word after `for NAME` and after `case WORD` | needs the previous words (Phase 2's splitter, or a 20-line special case first) |
| `{` `!` | command position | `{` needs a following blank (`{ ` is inserted) |
| `[[` , `time` | not in shish; do not offer |  |

Rule for this phase: offer the full set whenever the word under the cursor is
in command position and non-empty (bare TAB still lists files only). No
state tracking yet — Phase 4 adds it, and keeps this list as the fallback
when the tracked state offers nothing, so `fi<TAB>` can never be worse than
here. Tests: one assertion per word, plus `for x i<TAB>` → `in`.

### Phase 2 — quote- and escape-aware words; correct insertion

`compl_words()` replaces the "back to the previous blank" scan:

- Splits the line prefix into words honouring `'…'`, `"…"`, `\x`, `$(…)`/
  `` `…` `` nesting depth, and operators `; & | ( ) < > &&  ||`; each word
  keeps `raw` (as typed) and `cooked` (quotes removed) and the **open quote**
  at the cursor (`0`, `'`, `"`).
- Candidates are matched against `cooked`; insertion re-quotes according to
  the state at the cursor: unquoted → backslash-escape blanks/metacharacters
  (`my file` → `my\ file`); inside `"…"` → escape only `" $ \ ``; inside
  `'…'` → close the quote and reopen for a `'` (`'\''`). A unique match
  finishes the quote (`"my file" `), except with `nospace`.
- Directory part: expand `~`, `~user` (`expand_tilde_lookup`) and `$VAR` for
  the directory lookup only; the line keeps what the user typed (as `~/`
  does today).
- Trailing character rules in one place: dir (`stat`, follows symlinks) → `/`,
  file → space, `nospace` → nothing, inside an open quote → the closing quote.
- Listing: directories are shown with a trailing `/`; sorted with `strcmp`
  (C locale) — locale collation is out of scope; width by
  `mb_cols` once Goal 7 exists.
- First TAB inserts the common prefix; only if nothing was inserted (or on a
  second TAB) print the list, like bash; more than `COMPLETION_QUERY_ITEMS`
  (default 100) candidates asks "Display all N possibilities? (y or n)".
- Fixes `BUGS: term-complete-quoted-word` (the `ls "my fi|` and
  `ls my\ fi|` rows above both silently do nothing today); remove the entry
  and add a `tests/fixed.sh` case with the fix.

### Phase 3 — context and sources (what to complete where)

**Contexts** (from the word list; the classification is a `switch` on the
previous word / operator, not a parser):

| Context | Recognised by | Sources |
|---|---|---|
| command | first word; after `; & \| && \|\| ( ` ` { ! then do else elif if while until` | keywords (Phase 1), builtins, functions, aliases, `$PATH` executables (below) |
| command prefix | previous word is `command` `exec` `nohup` `env` `xargs` `time` `nice` `sudo` `builtin` `exec` | as *command*, one word later |
| redirection target | previous word matches `[0-9]*[<>]`, `>>`, `<&`, `>&`, `<>`, `>\|` | files; **no** `$PATH` |
| variable name | after `$`, `${`, `${#`, `unset`/`export`/`readonly`/`read`/`local`/`typeset` operands, and `NAME=` left of `=` | variables (`var_hsearch`); `$`/`${` insert braces only if a `}` context |
| assignment value | after `NAME=` | files (`x=~/pr|`) |
| tilde | `~foo|` | users via `expand_tilde_lookup`-style lookup (`getpwent` where libc has it; skip on dietlibc/WASI) |
| job | `%|` , operands of `fg bg wait kill disown` | `job_list`: `%1`, `%+`, `%-`, `%name` |
| signal | `kill -|`, `kill -s |`, `trap … |` | `sig_name()` table: `INT`, `SIGINT`, numbers |
| directory | operands of `cd pushd popd rmdir` (and `mkdir -p` parents) | directories only; `cd` also `$CDPATH` |
| option | word starting `-` after a builtin that has a `struct builtin_cmd.args` string | flag letters parsed out of `args` (`"[-lp] [[arg] signal_spec ...]"`); `--help` |
| `set -o`/`+o` | | option names from the `set` table (`builtin_set.c`) |
| `type`/`command -v`/`which`/`hash`/`help`/`unalias`/`alias` | | commands / commands / commands / commands / builtins / aliases |
| `in` (after `for NAME`, `case WORD`) | | the keyword `in` |
| default | everything else | files |

**`$PATH` executables** (new; nothing completes commands from `$PATH` today):
a cache keyed by the `PATH` string and each directory's `st_mtime`;
each entry is name only (no `stat` per file — `opendir` + `d_type`/`access(X_OK)`
lazily on first use of a prefix, then reuse); cache is dropped when `PATH`
changes or `hash -r` runs. Budget: first TAB on a `$PATH` of ~3000 commands
must stay under ~30 ms; measure (`How to measure` below). Entries already in
`exec_hashtbl` are a subset and add nothing.

**Defaults are `struct compspec` rows** (table above, one row per builtin
family), so the engine has a single dispatch path; per-row cost ≈ 40 bytes.

### Phase 4 — which reserved words are valid here (state across lines)

Goal: `fi<TAB>` only when an `if` is open; `then<TAB>` after `if list;`; `do<TAB>`
after `while list;` or `for … in …;`; `done<TAB>` inside a loop body;
`esac<TAB>` inside `case … in`; `else`/`elif` in a then-part; `}` inside `{`.

Three ways to know the open constructs, compared:

| | Idea | Multi-line | Same line | Cost/risk |
|---|---|---|---|---|
| A | mini-lexer over `pending + line[:cursor]` with a construct stack | needs `pending` (below) | yes | ~150 lines, independent of the parser; can drift from the real grammar (aliases that expand to keywords, `case` patterns, here-docs) |
| B | instrument the live parser: a global stack pushed/popped in `parse_if/loop/case/for/grouping/function` | yes, exact | **no** — tokens of the current line are not parsed yet | touches 6 parser files; still needs A for the current line |
| C | real parser dry run over `pending + line[:cursor]` with `P_COMPLETE` + silent errors; `parse_expect()` already receives the set of acceptable tokens (`toks`) | yes | yes, exact | needs silent `parse_error`, an EOF-at-cursor mode that returns the expected set instead of failing, alias-expansion side effects contained; ~120 lines in `src/parse/`, highest risk |

**Recommendation: A first**, C only if A's drift shows up in practice.

- `pending`: the editor keeps the text of the previous lines of the
  *current unfinished command* (`term_pending`, appended when a line is
  submitted while `prompt_number == 2`, cleared when the parser finishes a
  command or on ^C). No parser change; a few lines in `term_read.c`.
- The mini-lexer reuses `compl_words()` and adds a **construct stack**
  driven by the reserved word in each command position:
  `if`→IF_COND, `then`→IF_THEN, `elif`→IF_COND, `else`→IF_ELSE, `fi` pops;
  `while/until`→LOOP_COND, `for`→FOR_HEAD, `do`→LOOP_BODY, `done` pops;
  `case`→CASE_HEAD, `in`→CASE_BODY, `esac` pops; `{`→BRACE, `}` pops;
  `(`→SUBSHELL, `)` pops; `name()`/`function`→FUNC_HEAD. Words inside `case`
  patterns, quoted strings, `$( )` and here-doc bodies (`<<EOF` … `EOF`) are
  skipped, not interpreted.
- Valid-next table (the stack top decides):

| Top of stack | Command-position words offered |
|---|---|
| none / BODY states | starters: `if while until for case function { ! ( ` and commands |
| IF_COND, LOOP_COND | commands, plus `then` / `do` **only after a list terminator** (`;` `&` or newline) |
| IF_THEN | commands, `elif`, `else`, `fi` |
| IF_ELSE | commands, `fi` |
| LOOP_BODY | commands, `done` |
| FOR_HEAD | after `for NAME`: `in` or `do` (`;`/newline first) |
| CASE_HEAD | `in` |
| CASE_BODY | `esac`, patterns (no completion) |
| BRACE | commands, `}` |

- `alias unless=if`: if the first word of a command is an alias whose
  first word is a keyword, treat it as that keyword (one level, via
  `parse_aliases`).
- Fallback: when the valid set contains nothing matching the typed prefix,
  offer **all** reserved words anyway (Phase 1 behaviour), so a lexer error can
  never remove a completion that worked before.

### Phase 5 — extensibility: `complete`, `compgen`, `compopt`

A bash-compatible **subset** (so existing completion scripts with
`complete -F` mostly work), off-by-default builtins in `EXTRA_BUILTINS` until
the size is known (`ENABLE_COMPLETE`); the editor side (Phases 0-4) does not
depend on them.

**Builtins**

| Command | Behaviour |
|---|---|
| `complete [-abcdefgjksuv] [-o opt]... [-A action]... [-F func] [-C cmd] [-W words] [-X filter] [-P prefix] [-S suffix] name...` | register a spec for each `name` |
| `complete -D` / `-E` / `-I` | default spec / empty-line spec / initial word (first word being completed) |
| `complete -p [name...]` | print registered specs in re-usable form (`complete -F _foo foo`) |
| `complete -r [name...]` | remove specs (no name: all) |
| `compgen [options] [word]` | print the candidates the given spec would give for `word`, one per line; exit 0 if any, 1 if none; same options as `complete` (no `name`) |
| `compopt [-o opt] [+o opt] [name...]` | change options of a registered spec, or of the completion in progress when called from `-F` |

Actions (`-A`): `alias builtin command directory export file function
hostname job keyword running setopt signal stopped user variable helptopic`
(`arrayvar binding disabled enabled group service shopt` are not planned:
no arrays/readline/`enable`/`shopt` in shish; `-A` with an unsupported action
is an error, exit 2). Letter forms as bash (`-a -b -c -d -e -f -g -j -k -s
-u -v`). `-o`: `default dirnames filenames noquote nosort nospace plusdirs`
(`bashdefault` accepted and ignored).

**Dispatch** (first match wins): spec registered for the exact command word,
then for its basename, then `-E` (empty word on an empty line), then `-D`,
then the built-in default row (Phase 3). The "command word" is the first word
of the simple command after prefix words (`sudo`, `command`, …, from Phase 3).
`-o default` falls back to file completion when the spec yields nothing.

**Function protocol** (`-F func`), adapted to a shell without arrays:

- Called as `func cmd cur prev` (`$1` command name, `$2` word being completed,
  `$3` the word before it), as bash.
- Variables set for the call: `COMP_LINE` (whole line), `COMP_POINT` (cursor
  byte offset), `COMP_CWORD` (index of the word being completed, from 0),
  `COMP_WORDS` (**newline-separated** words — deviation: no arrays),
  `COMP_TYPE` (`9` normal, `63` list on ambiguity), `COMP_KEY` (`9`).
  They are unset again afterwards.
- Result: the function sets `COMPREPLY` to a **newline-separated** string,
  or `compgen … ` writes to stdout and the function runs
  `COMPREPLY=$(compgen -W "$words" -- "$2")` — the idiom works unchanged
  apart from the missing array syntax. An empty/unset `COMPREPLY` means no
  candidates (then `-o default` applies).
- If shish later grows arrays, `COMPREPLY`/`COMP_WORDS` become arrays and the
  scalar forms stay accepted; note it here then.

**Running shell code from the editor — the risky part** (each item is a
test):

- **Preserve state**: save and restore `$?`, `$_`, positional parameters,
  `set -e/-u/-x` flags, `LINENO`, the current `PIPESTATUS`-equivalent, the
  pending `fdstack`, and trap-in-progress; the function runs with stdout
  and stdin on `/dev/null` and **stderr discarded** (bash lets stderr
  through and messes up the line; we do not).
- **Terminal**: the editor is in raw mode; the function must run with the
  cooked attributes restored only if it wants to read (`read` in a
  completion function is not supported: stdin is `/dev/null`).
- **Runaway**: a function that loops forever hangs the prompt. ^C arrives as
  the byte 3 (raw mode), so poll the input for it between evaluated
  commands (a hook in `eval_list`, guarded by a `compl_running` flag), or
  set an `alarm()` guard (default 2 s, `COMPLETION_TIMEOUT`); either aborts
  the function and beeps.
- **Re-entrancy**: a completion function invoked while another is running
  (a `-F` calling `compgen` is fine; a TAB is not reachable) — assert with a
  `compl_running` counter.
- **`-C cmd`** runs an external program: needs `fork`; not available on WASI
  (the spec registers, generation fails with a diagnostic once).
- **Word list expansion** (`-W`): expanded when the candidates are generated,
  not when registered (bash), so `$(…)` in a word list runs on every TAB:
  documented, same trust level as `-F`. Data from the command line
  (`COMP_WORDS`, `$2`) is never `eval`ed by the engine.
- **Exit and errors**: an undefined `-F` function or a non-zero exit
  status with no `COMPREPLY` gives no candidates silently; a diagnostic only
  under `set -x`-style debug (`COMPLETION_DEBUG=1`).

**Storage**: a singly linked list of `struct compspec` in `src/complete/`
(`complete_spec_add/find/remove/print`), names lowercase-exact, strings
`str_dup`ed; a subshell sees the parent's specs and changes made there do
not leak (same treatment as functions, `exec_functions_save/restore`).

**Loading completions lazily** (optional, later): `complete -D -F _loader`
where `_loader` sources `$SHISH_COMPLETION_DIR/$1.sh` on the first TAB for
command `$1` (bash-completion's model). Ships as an example under `doc/`,
not compiled in.

### Phase 6 — usability polish (each independent, any order)

- Cycling: a second TAB with no unique prefix steps through the candidates
  (`menu-complete`), shift-TAB (`ESC [ Z`) goes back; toggle
  `COMPLETION_MENU=1`.
- `COMPLETION_IGNORE_CASE=1` (match, keep the candidate's case), a
  shell-variable substitute for readline's `completion-ignore-case`.
- Colour the list by type when `LS_COLORS`/`NO_COLOR` allow (dir, exec,
  symlink) — needs `lstat` per shown entry, only for lists ≤ the query
  threshold.
- `ESC ?` lists without inserting; `ESC *` inserts all matches (both readline).
- Paging of a long list at `LINES` rows (`--More--`).
- `WINDOWS_NATIVE`: a `FindFirstFile` source so the editor is not
  completion-less there; separator `\`, case-insensitive match.
- History-word completion (`ESC /` / `ESC .`, insert last argument) —
  belongs to the line-editor rewrite (see "Also open"), not here.

### Phase 7 — optional: exactness through the real parser (Option C)

Only if Phase 4's lexer produces wrong offers in practice (aliases expanding
to compound commands, here-docs, `case` inside `$(…)`): add `P_COMPLETE`
to the parser flags, a silent `parse_error()`, and make `parse_expect()`
record its `toks` argument and return when the cursor offset is reached;
`compl_context()` then asks the parser instead of the lexer. Measure both
against the same table before switching.

### Testing

- **Unit (no pty)**: `compgen --line 'LINE' [POS]` (dev switch, Phase 0)
  drives contexts and sources; table rows `line|pos|expected candidates`
  in `tests/term-complete.sh`, one `assert_equal` per row — hundreds of
  rows cost milliseconds.
- **Quote/escape matrix** (Phase 2): every combination of open quote ×
  special character in the name (`space ' " \ $ ` * ? [ ] ( ) ; & | < > ~ !
  newline tab`) — insert, then `eval` the result and compare with the
  file name; this is the only way to be sure the escaping is right.
- **State machine** (Phase 4): the valid-next table as a test, plus a
  random walk: generate a valid shell fragment (the Goal 8 fuzzer, when it
  exists), cut it at a random word boundary, and assert that the
  continuation's next keyword is among the offers.
- **`complete`** (Phase 5): every option, `-p` round trip
  (`complete -p | eval` reproduces the registry), `-F` with `COMP_*`,
  runaway function aborted by the guard, `$?` preserved, a `-F` that
  writes to stderr leaves the line intact (pty case), subshell isolation.
- **Editor glue** (pty, few cases): insertion, redraw after a list, cursor
  in the middle of a line (text after the cursor is kept and not
  considered), two-TAB list behaviour.
- **Fuzz**: random bytes as the line and a random cursor position under
  ASan+UBSan (Goal-8 style gate): `compl_words()` and `compl_context()`
  must never read outside `term_cmdline`.

### Size and cost (estimates; calibration: today's `term_complete.c` is
363 lines / ≈3 KB)

| Piece | Lines | Code |
|---|---|---|
| Phase 0-2: words, set, apply, quoting | ≈450 | ≈4 KB |
| Phase 3: contexts, sources, `$PATH` cache, defaults table | ≈500 | ≈5 KB |
| Phase 4: pending + construct stack | ≈200 | ≈2 KB |
| Phase 5: registry, `complete`/`compgen`/`compopt`, running `-F` | ≈600 | ≈6 KB (only with `ENABLE_COMPLETE`) |
| tests | ≈400 | — |
| **Total** | **≈2100 lines** | **≈11 KB without, ≈17 KB with Phase 5** |

The completer only exists in interactive builds; the `WITH_COMPLETE`
switch (default on with the line editor, off for the WASI/`-c` builds that
never read a tty) keeps a non-interactive build unchanged — verify with the
stripped-size comparison from Goal 5.

### Order of work (each step its own change with test and this file updated)

1. **Phase 1** first: it is the smallest visible change and needs no
   refactor (`term_complete.c` keyword list + `in` special case + tests).
2. **Phase 0**, then **Phase 2** (correctness first: quoting bugs make every
   later feature worse), then **Phase 3**.
3. **Phase 4** once Phase 3's word list exists (it reuses it).
4. **Phase 5** last of the core: everything before it is useful without it,
   and it is the one with the state-preservation risk.
5. Phases 6-7 as wanted.

### Risks and open questions

- **Arrays**: the `complete -F` protocol above is a scalar approximation.
  Existing bash completion scripts use `${COMP_WORDS[COMP_CWORD]}`,
  `COMPREPLY=( $(compgen …) )` and `local -a`; those will not parse in
  shish. Compatibility with the bash-completion package is **not** a goal;
  compatibility with the *idioms* in this section is.
- **Default builtins**: `complete`/`compgen` in `DEFAULT_BUILTINS` or
  `EXTRA_BUILTINS`? Decide from the measured size (≈6 KB, Goal 5.5 policy).
- **Correctness of the lexer vs. the parser**: Phase 4 accepts small drift;
  the fallback keeps it harmless. Track drift cases in `BUGS` as they are found.
- **`$PATH` scan cost** on slow/network directories: the cache is per
  directory mtime, but the first scan blocks the prompt; a soft limit
  (`COMPLETION_PATH_MAX`, default 5000 entries) and no scan of directories
  that are not `stat`-able within one call.
- **UTF-8** (Goal 7): candidate width in the listing and the insertion of a
  common prefix must not cut a multibyte character — use `mb_clen` when the
  common prefix is computed byte-wise.
- **Security**: `-W`/`-F` execute user-defined code on TAB; nothing typed at
  the prompt is ever `eval`ed by the engine itself. A file name that looks
  like `$(cmd)` must be inserted escaped (part of the Phase 2 matrix).
- **Interrupted long operations**: `opendir` on a hung NFS mount blocks the
  prompt; no fix planned (same in bash), note in the docs.

### How to measure

```sh
# candidate generation without a terminal (Phase 0 switch)
time (for i in 1 2 3 4 5; do build/x86_64-linux-gnu/shish -c "compgen --line 'p'" >/dev/null; done)
# $PATH scan, cold and warm
build/x86_64-linux-gnu/shish -c 'compgen --line "a" | wc -l; compgen --line "a" | wc -l'
# code size of the completer (interactive vs -c-only build), stripped
size build/x86_64-linux-gnu/CMakeFiles/libshell.dir/src/term/*.o build/x86_64-linux-gnu/CMakeFiles/libshell.dir/src/complete/*.o
```

---

## Goal 10 (secondary) — `cp` and `mv`: what is left

`src/builtin/core/builtin_cp.c` holds `builtin_cpmv()`; the `cp` and `mv` rows (`BUILTIN_CP`/`BUILTIN_MV`,
`EXTRA_BUILTINS`, off by default) point at it, and `cmake/Builtins.cmake` adds `builtin_rm.c` when `mv`
is on (`builtin_rm_tree()` is the exported `rm -r` walk). Tests: `tests/builtin-cp.sh` (which hangs,
`BUGS: builtin-cp-sh-hangs`), `tests/builtin-mv.sh` (the `EXDEV` cases run when `/dev/shm` is another
file system).

Open:

- `-T`/`-t`; the `SHISH_CPMV_FORCE_COPY` test hook; messages in GNU wording (`target 'x': No such file...`
  instead of `cp: x: not a directory`).
- **Interrupting a copy:** an interactive shell ignores `SIGINT`, so a running `cp` of a large file
  cannot be interrupted; poll for it. Also `ENOSPC` handling and a 4 GiB sparse-file check (skip
  when the file system lacks holes), then the optional `copy_file_range()` fast path if the measured
  copy speed matters; hole preservation.
- **Portability pass:** WASI (Node harness), `cfg-mingw64` (`WINDOWS_NATIVE` rename/attributes; the
  mingw sysroot headers are not installed here, so it is only guarded with `CPMV_NOUNIX`, never
  compiled), musl/diet builds.
- **Size:** measure the pair against two separate entry points (`-DBUILTIN_CP=ON -DBUILTIN_MV=ON`,
  `strip`, `stat -c%s`) and keep the smaller layout.
- Documentation: `doc/builtins.md` entries, short `help_cp`/`help_mv` (Goal 5.2 counts help bytes),
  README builtin list.
- `ln` used to unlink an existing destination: `cp`/`mv` must not do that either (only with `-f`).

---

## Goal 11 (secondary) — `sed` and `awk`: what is left

Both are `EXTRA_BUILTINS` (off by default) on the shared engines `text/sed/`, `text/awk/` and
`text/dfa/` (`dfa_replace`/`dfa_repl`), `lib/arena` and `lib/hashmap`; tests `tests/builtin-sed.sh`,
`tests/builtin-awk.sh`.

Documented omissions (each is in the `help_*` text, so not a `BUGS` entry):

- **`sed`:** `-i`, `-s`, `-z`, `e F z W R M`, `0,/re/`, `addr,+N`, `first~step`, the `q`/`Q` exit-code
  arguments, and the `y` escapes other than `\n`, `\\` and the delimiter.
- **`awk`:** `cmd | getline`, `print | cmd` and `system()` are parsed and dispatched through
  `struct awk_io.run_shell`, but `builtin_awk.c` leaves it unset (a clean runtime error); wire it to
  the shell's evaluator (the `$(...)`/`eval` machinery) as a self-contained follow-up. `RS=""`
  (paragraph mode) and a regex `RS` are not implemented (`RS` is one byte). `length`, `substr`, `index`,
  `match`, `printf %c` count bytes (conformant in the POSIX locale; character semantics wait for Goal 7).
  `for (k in a)` order is bucket order (unspecified in POSIX).

---

## Goal 12 (secondary) — more `EXTRA_BUILTINS`: POSIX utilities real scripts call most, still external

**Not started; this section is the candidate list.** The rationale is the same as for `cp`/`mv`: a
builtin is free where an external binary already exists (it only wins the `PATH` lookup +
`fork`+`exec`) and a real capability where none does (WASI, a from-scratch container,
`-DLINK_STATIC=ON` single-file image).

**Evidence.** A histogram of every command word bash actually dispatched across a large corpus of
real-world shell scripts (`../plot-cv/shell-commands-histogram.txt`, 3545 distinct names), filtered to
POSIX.1-2024 utilities that are still not shish builtins:

| Count | Utility | Count | Utility | Count | Utility |
|---|---|---|---|---|---|
| 794 | `diff` | 73 | `dd` | 12 | `nohup` |
| 412 | `tput` | 37 | `chown` | 10 | `nice` |
| 267 | `getconf` | 29 | `du` | 10 | `fold` |
| 228 | `cmp` | 26 | `od` | 8 | `tty` |
| 75 | `bc` | 20 | `df` | 6 | `mkfifo` |
| 19 | `comm` | 5 | `join` | 4 | `expand` |

(`seq`, `install`, `dir`, `yes`, `stat`, `groups`, `arch`, `stty`, `sum`, `sync`, `nproc`, `truncate`,
`fmt`, `base64`, `mknod` appear at similar frequencies but are **not** POSIX utilities, so they stay
out of scope per the "design spec is POSIX" rule.)

- **`diff`**: real value (794 uses) but by far the biggest: a line diff needs an LCS/Myers engine,
  closer in scope to `text/dfa` than to one `builtin_*.c`; give it its own `text/diff/` the way `sed`
  got `text/sed/`, and its own Goal if it is picked up.
- **System/identity utilities** (`chown`, `du`, `df`, `dd`, `nice`, `nohup`, `tty`): individually small
  (mostly one syscall plus formatting) but only pay off where `fork`+`exec` of the real one is
  unavailable.
- **Niche/legacy** (`getconf`, `cmp`, `bc` (non-trivial), `comm`, `fold`, `mkfifo`, `join`, `expand`,
  `od`, `pr`, `cksum`, `tsort`, `csplit`, `pathchk`, `chgrp`): candidates, not a near-term plan; no
  per-utility sizing has been done. The sized schedule for the ones in Goal 16 is there.
- Known gaps in the builtins that already moved: `sort` keeps everything in memory and compares bytes (no locale collation); `tail -f` follows one
  file; `split` has the POSIX options only.

No file layout, option sets, or size estimates have been worked out for any of these yet: that is the
next step once one is picked up (POSIX page -> option table -> LOC estimate -> `BUGS` entries for any
deliberately omitted option).

---

## Goal 13 (secondary) — pull-based filter chaining: what is left

`eval_pipeline()` chains a pipeline's whole non-last prefix straight into the true last stage through
in-process buffers (`FD_FILTER`, `struct filter_ops`, `pipeline_filter_prepare_chain()`) when every
non-last stage is a filter-capable builtin with literal argv; a runtime decline (`grep -c`/`-q`) rolls
the chain back and the pipeline forks as before. No `fork()`/`pipe()` runs for `cat file | grep -E '(a|b)' |
sed '...'`. Both `sed` and `grep` are optional builtins, so nothing outside
`src/builtin/{extra,filter}/builtin_*.c` may name them, and every build must still compile with either
or both off.

Open:

- **`!HAVE_FORK`/WASI builds get correctness, not just speed, from this.**
  `eval_pipeline_sequential()`'s per-stage full-materialization fallback (`eval_pipeline.c`) hangs on an
  infinite producer (`yes | sed ... | head` never finishes stage one). An `FD_FILTER` chain needs no
  `fork()`/`pipe()` for its own stretch, so decide whether `eval_pipeline_sequential()` should try
  chaining first and fall back to full materialization only when the chain is not entirely steppable
  builtins. It does not attempt chaining yet.
- `BUGS: filter-chain-hides-data-from-external-command`: an external command inside a function or `{ }`
  last stage reads the real fd 0 and gets nothing.

### Which builtins would benefit from being a filter (2026-09-27)

Filter-capable today: `cat`, `grep`, `sed`, `sort`, `head`, `tail`, `uniq`, `cut`, `nl`, `tr`, `paste` and the compress/uncompress family. A chain needs *every* non-last stage to be one (the last
stage already runs in-process and reads the chained buffer), so a single non-capable builtin anywhere
in the prefix (`echo x | awk ... | sed ...`) sends the whole pipeline back to `fork()`+`pipe()`.
What matters is therefore position: **producers** (first stage, no stdin) and **middle stages**.

| builtin | role | verdict | how |
|---|---|---|---|
| `echo`, `printf` | producer | **highest value**: `echo "$x" \| grep ...`, `printf '%s\n' ... \| sort` are the most common first stages of all | eager (see below), ~10 lines each |
| `awk` | middle | **high**: `grep ... \| awk ... \| sed ...` is a standard idiom; `text/awk` already reads through `fd_in` | streaming, like `sed`'s filter (`awk_state` reads records) |
| `tee` | middle | **high**: `cmd \| tee log \| next`; side effect (write files) plus pass-through | streaming: copy in `read()`, write the files as bytes pass |
| `find`, `ls` | producer | medium: `find . -name '*.c' \| grep ...`, `ls \| wc -l` | eager |
| `set`, `alias`, `export -p`, `readonly -p`, `trap`, `type`, `command -v`, `jobs`, `umask`, `pwd` | producer | medium: `set \| grep ^X`, `alias \| sed`, `jobs \| wc`; read-only views of shell state | eager |
| `basename`, `dirname`, `realpath`, `readlink`, `which`, `uname`, `hostname`, `expr` | producer | low: normally used inside `$(...)`, not a pipeline | eager, free once the adapter exists |
| `wc`, `digest` | sink | low as a filter: a sink emits one line at EOF, and as the *last* stage it already chains; only `x \| wc -l \| y` benefits | eager |
| `xargs`, `timeout`, `env`/`nice`/`nohup` (planned) | runs another command | **no**: the output belongs to the executed command (real fds, forked child), nothing to hand back in-process | none |
| `cd`, `read`, `export`, `set` (assigning forms), `.`, `eval`, `exit`, `mktemp`, `sleep`, `kill` | state / no stdout | **no**: side effects on the shell must happen in the shell, a chained stage runs lazily and possibly never | none |
| `date`, `id` (builtins, not filters yet); planned: `du pathchk` | producer | as `find` | eager |

**One adapter covers every "eager" row.** A generic `filter_eager` ops table in
`src/builtin/builtin_filter.c` runs the builtin's normal entry point on the first `read()` with `fd_out`
redirected into a `stralloc` (the same `FD_SUBST` mechanism `$(...)` uses) and, for a consumer, `fd_in`
set to the upstream buffer; then `read()` hands the bytes out and `status()` returns the exit code.
A builtin opts in with one table entry (`&filter_eager`), no per-builtin code. Trade-offs: the whole
output is held in memory, and an eager stage does not stop early (`yes | head` would never end, so
`yes` stays out); real streaming stays with `cat grep sed head ...` (and `awk`, `tee` once they are filters). It must decline (`open()`
returning NULL, nothing printed) for anything that changes shell state, which is the "no" rows above.

---

## Also open (secondary)

- **WASI build (`cfg-wasi`, `doc/wasm.md`) — builds and runs (Node, webassembly.sh).** `build/wasi/shish` is 248 KB and runs under Node's WASI
  (`--experimental-wasm-exnref`): 18 of 34 `tests/*.sh` pass unmodified,
  the rest need fork/external commands or job control. Open:
  - **webassembly.sh works** (Chrome 152, upload via `wapm upload`,
    2026-09-19). The module needs WebAssembly exception handling
    (`exnref`; wasi-sdk 34 no longer emits the legacy opcodes); an older
    browser lacking it cannot load the module — the fallback would be a
    build without EH, i.e. replacing the `setjmp` unwinding in
    `src/eval/` (design-sized). The interactive prompt there is untried.
  - External commands cannot run; a wasm-hosted "run this program" hook
    (webassembly.sh runs other `.wasm` commands by name) would need a
    host import, not an `execve`.
  - `tests/fixed.sh` hangs under WASI (job-control cases); needs a
    WASI skip list.

- **Line-editing/terminal-abstraction/key-bindings rewrite** — a
  design-sized project inherited from the old `TODO` file, not a fixable
  bug. Minimal tab-completion (`src/term/term_complete.c`: filenames, plus
  keywords, builtins and functions at a command's first word) is the
  only piece of this done so far; its growth path (context, `$PATH`,
  `complete`/`compgen`) is Goal 9. UTF-8-aware editing (per-character cursor and
  backspace, display columns) is milestone M5 of Goal 7.

---

## Goal 14 (secondary) — evaluator trace (`SHISH_TRACE`): what is left

The trace layer (`src/trace.h`, `src/trace/`; design and event catalogue in
[`doc/debug-output.md`](doc/debug-output.md), method in `CLAUDE.md` "Debugging with TRACE()")
is complete for the modules `exec builtin fd fdstack fdtable eval expand redir var sh job sig parse`.

Open:

- **`SHISH_TRACE` is read from the process environment once**, at the first event; an
  `export SHISH_TRACE=...` inside a running script is not seen. Reading it through `var_get` would
  fix that but touches every event's startup path.
- **Autotools:** works in-tree only (`./autogen.sh && ./configure --enable-debug CPPFLAGS=...`, serial
  `make`, then `./config.status src/builtin_config.h` once: configure does not run its
  `AC_CONFIG_COMMANDS` step, cause not found). The `src/*/Makefile.in` `MODULES` lists are
  hand-maintained and drift.
- `var.import` is not traced on purpose (one line per environment variable; `var.export` reports the
  count).
- `timeout` forks a function so it can be killed, and then its output is not captured in `$(...)`
  (`BUGS: timeout-function-output-not-captured-by-command-substitution`).

---

## Goal 15 (secondary) — vi mode (`src/term/term_vimode.c`): gaps against vim/POSIX `set -o vi`

Tests: `tests/term-vi.sh`. `vi` is always on: there is no `set -o vi` / `set -o emacs` switch.

**Missing, roughly by how often they are used at a shell prompt**

- **Undo and repeat:** `u`, `U`, `.` (repeat last change), `Ctrl-R`-style redo (`Ctrl-r` is the history search).
- **History:** `G` / `[n]G` (go to entry n), `n` / `N` (repeat the last search), `?` (forward search; `/` today calls the emacs-style `Ctrl-R` incremental search, not a vi `/pattern<CR>` prompt), `+` / `-` (like `j` / `k`), `#` (comment the line out and enter it).
- **Editor hand-off:** `v` (open the line in `$VISUAL` / `$EDITOR`; POSIX requires it).
- **Motions:** `|` (column), `%` (matching bracket), `g_`, `ge` / `gE`, `H` / `L` / `M`, `{` / `}` / `(` / `)` (irrelevant on one line, but `d}` etc. would be used in multi-line entries), `_`, `-` / `+`.
- **Operators:** `~` (toggle case), `g~` / `gu` / `gU`, `>` / `<` (no meaning on one line), `J` (join), `!`, `=`, `gq`.
- **Text objects:** `iw` `aw` `iW` `aW`, `i"` `a"` `i'` `a'`, `i(` `a(` `ib`, `i[` `i{` `it` — `ciw`, `di"` and `ci(` are the most missed.
- **Registers and marks:** named registers (`"ayw`, `"ap`), the numbered delete history (`"1p`), `m{a-z}` / `'x` / `` `x ``; there is a single unnamed yank register only.
- **Insert mode:** `o` / `O` (meaningful only for multi-line), `gi`, `Ctrl-w` (delete word), `Ctrl-u` (delete to line start), `Ctrl-v` (literal), `Ctrl-t` / `Ctrl-d` (indent), `Ctrl-[` (as ESC), `Ctrl-o` (one command), `Ctrl-h` as backspace, `Ctrl-n` / `Ctrl-p` completion (`Tab` is the only completion).
- **Visual mode:** `v` / `V` / `Ctrl-v` and their operators; nothing exists.
- **Search inside the line:** `*` / `#` (word under the cursor), `/` and `?` within the current line.
- **Counts:** missing are counts for `i a I A` (`3ix<ESC>` repeats), for `p` / `P` (`3p`), `.` and `~`.
- **Repeat find:** `;` / `,` work, but `t` repeated with `;` does not skip past the adjacent character the way vim does when `cpo` has no `;`.
- **Pending-state display:** no `-- INSERT --` / `-- NORMAL --` indicator, no cursor-shape change (`\e[2 q` block / `\e[6 q` bar), no display of a pending operator or count.
- **Escape handling:** a lone ESC waits 50 ms for a following key (fixed timeout, not `ttimeoutlen`); there is no way to configure it.
- **Multi-line entries:** history entries with embedded newlines are shown on one line; there is no line-wise `j` / `k` / `o` / `dd` inside them.
- **Wide characters:** columns are bytes, so `l` / `x` / `w` step through a UTF-8 sequence one byte at a time.
- **Options:** none of `set -o vi`, `EDITRC` / `inputrc`-style key rebinding, `bind`, or a `vi`-mode `KEYTIMEOUT` is honoured.

---

## Goal 16 (tertiary) — the remaining POSIX utilities as optional `EXTRA_BUILTINS`

Utilities from the POSIX utilities volume that are not builtins yet. None is needed by the shell itself,
so **this waits until the main quest is done.** Each one is opt-in (`BUILTIN_<NAME>`,
`cmake/Builtins.cmake`, off by default), lives in `src/builtin/extra/builtin_<name>.c`, follows its POSIX
page as the design specification (<https://pubs.opengroup.org/onlinepubs/9799919799/utilities/>; a
missing option is a `BUGS` entry with a repro), uses `lib/` instead of libc stdio/string, and comes
with `tests/builtin-<name>.sh` and a `help_<name>` text.

### Schedule, simplest and smallest first

Complexity: **1** one syscall or one loop, **2** a small parser or a few options, **3** real algorithm
or state, **4** terminal / `/proc` / many output formats, **5** layout engine. Lines are C lines for
the builtin alone, calibrated on what exists (`tee` 95, `mkdir` 105, `wc` 165, `ls` 274, `xargs` 319,
`touch` 326 with its date parser); tests add roughly the same again.

| # | utility | cx | lines | total | what it takes |
|---|---|---|---|---|---|
| 1 | `mkfifo` | 1 | ~45 | ~45 | `mkfifo(3)` plus the mode parser (`-m`) that `mkdir` already has; move the parser to `lib/` first |
| 2 | `nice` | 1 | ~55 | ~100 | `nice(2)`, then `exec_command()`; only the increment parsing (`-n`, and the obsolescent `-10`) |
| 3 | `nohup` | 2 | ~70 | ~170 | ignore `SIGHUP`, redirect stdout/stderr to `nohup.out` if they are terminals, `exec_command()`; exit 126/127 |
| 4 | `renice` | 2 | ~80 | ~250 | `setpriority(2)` over `-p`/`-g`/`-u` ID lists; no exec; `-n` is required in POSIX.1-2024 |
| 5 | `pathchk` | 2 | ~90 | ~340 | `pathconf(3)` limits, `-p` portable-character check, `-P` empty/leading-hyphen check |
| 6 | `du` | 3 | ~140 | ~480 | directory walk like `rm -r`; `-a -s -k -x -H -L`, hard links counted once (`st_dev`/`st_ino` set) |
| 7 | `fuser` | 4 | ~130 | ~610 | Linux only: scan `/proc/*/fd`, `cwd`, `root`, `maps`; `-c -f -u`; not portable, no test on other systems |
| 8 | `more` | 4 | ~220 | ~830 | raw-terminal pager on `src/term/` (`term_init`, window size, `SIGWINCH`); commands `q space enter /pattern`; not a filter |
| 9 | `od` | 4 | ~260 | ~1090 | `-A -j -N -t` (a c d f o u x, sizes), `-v` duplicate folding, the obsolete `-b -c -d -o -s -x` aliases, address radix |
| 10 | `ps` | 4 | ~300 | ~1390 | Linux only: `/proc/*/stat`+`cmdline`, `-o` column formats, `-e -a -A -f -l -p -t -u -U -G -g -d`, tty name mapping |
| 11 | `pr` | 5 | ~320 | ~1710 | pagination, headers/footers, `-column`/`-m` multi-column layout, `-e -i -n -o -w -f -F -l -h -s -t -d -r -a` |

**About 1,700 lines for the 11 utilities still to write** (about twice that with tests). Stop after `renice`,
or after `du`, unless a real use appears.

### Not planned

- **`rmdel`** and the other SCCS utilities (`admin delta get prs sact sccs unget val what`): SCCS is
  obsolescent and dropped from newer POSIX editions. `rmdel` alone would need the whole SCCS file
  format (`admin`, `get`, `delta`, weave parsing): **1,500+ lines** for a version-control system
  nobody uses. Out of scope, like `lex`, `yacc` and `c99` (`CLAUDE.md`, "Design specification for
  builtin utilities").

### Shared code to extract first

Do these two before the utilities that need them; each removes a duplicate:

1. **mode-string parser** (`chmod`, `mkdir`, `mkfifo`): now inside `builtin_chmod.c`/`builtin_mkdir.c`, move to `lib/`.
2. **wrapper helper** for "set something up, then `exec_command()` the rest" (`env`, `nice`, `nohup`, and
   `xargs`/`timeout` already do it by hand): about 30 lines around `exec_hash()` + `exec_command()`.

Filters (`head uniq paste cut tr nl tail`, already builtins) use `src/builtin/builtin_filter.[hc]` (`filter_in`,
`filter_ops`: declarative `opts/size/option/setup/step/finish`, see the comment in the header) so they can join filter chains (Goal 13); `tail -f` and `more` cannot chain.
