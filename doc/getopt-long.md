# Long options and `--help` for the builtins: plan

Status: **implemented** (2026-10-14): phases 1-3 for every builtin whose option exists today, `date -d/-r/-I/-R/--rfc-3339`
(`lib/date/date_parse.c`), `touch`/`timeout`/`realpath` on the shared parser, `ls --color`. Tests: `tests/lib/getopt_test.c`,
`tests/lib/date_test.c`, `tests/builtin-help.sh`, `tests/builtin-longopts.sh`, `tests/fixed.sh` (fixes/406).
Not done: `ls --sort=`, `cp/mv -t/-T`, `realpath --relative-base`, `mktemp --suffix/--tmpdir`, `cat`/`sed` extras that
need a feature first (a long name is only added together with its feature), and the `-h` removal in `find`.

Size (MinSizeRel, `size`, text): default builtins 313973 -> 316362 (+2.4 KB, +0.8%); `ENABLE_ALL_BUILTINS` + find 619340 ->
642803 (+23.5 KB, +3.8%), which also carries `find -exec/-regex/-delete/...` and `date_parse`.

Original plan text follows. Every number below was counted, not estimated, unless it says "estimate".

Decisions taken (2026-10-14): **no argument permutation; exit status 2 on a bad option; only `--help` shows help (never `-h`);
`ls --color` is in, minimal.** Section 7 lists what is still open.

Goal: somewhere between POSIX/BSD utilities (short options only) and GNU (every option has a long twin, abbreviations,
argument permutation, `--version`, translated help). Scripts written for GNU `cp`, `date`, `realpath`, `grep`, ... should
run unchanged for the options people actually use, and `cp --recursive --verbose` should be allowed to replace `cp -Rv`
where a script wants to be read. Nothing more.

---

## 1. Where we are

| fact | where |
|---|---|
| one parser, short options only: `shell_getopt()` / `shell_getopt_r()` over `struct optstate` | `lib/shell/shell_getopt.c`, `lib/shell.h` |
| 52 source files call it (about 45 option strings), plus `filter_init()` for the 13 filters (`ops->opts`) | `grep -rl shell_getopt src` |
| `--help` is not understood anywhere: `ls --help` prints `ls: --: invalid option` (the parser takes `--help` as the option `-` and then reads `-`) | run it |
| `cp -h` is "invalid option"; `sleep --help` is "invalid time interval" | run it |
| three builtins hand-roll long options with an almost identical `extract_longopt()` (touch, timeout) or an inline copy (realpath `--relative-to`), each removes the option from `argv` **before** `shell_getopt` runs | `builtin_touch.c:46`, `builtin_timeout.c:52`, `builtin_realpath.c:35` |
| the text for `--help` already exists for **every** builtin: `struct builtin_cmd` has `.args` (synopsis) and `.help`, and `help cp` prints them | `src/builtin.h:11`, `src/builtin/builtin_help.c` |
| `builtin_invopt(argv)` is the one place that reports an invalid option; 47 call sites, all of the shape `default: builtin_invopt(argv); return 1;` (a handful return other values: `env` 125, `awk` 2, `dirstack` 2) | `src/builtin/builtin_error.c:48` |
| `-h` already means something in `grep` (no file names), `nl` (header style), `set`/`sh` (hashall). `test -h` is an operand, and the `h` in `sleep`/`timeout` is the hours suffix of the duration, not an option | `case 'h'` |
| `date` has only `-u` and `+format`; `touch -d` has a private parser for `@EPOCH`, `YYYY-MM-DD`, `YYYY-MM-DD[T ]hh:mm[:ss]` | `builtin_date.c`, `builtin_touch.c:138` |

The consequence that shapes the whole plan: **`.args` and `.help` make `--help` free**, and **`shell_getopt` is the one choke
point** every option-taking builtin already goes through. So `--help` belongs in the parser and in
`builtin_invopt`, not in 50 option tables.

---

## 2. Design

### 2.1 One parser, one new optional argument

```c
int shell_getopt(int argc, char* const argv[], const char* optstring);                    /* unchanged signature */
int shell_getopt_long(int argc, char* const argv[], const char* optstring, const char* longopts);
```

`shell_getopt(...)` becomes `shell_getopt_long(..., NULL)`. The existing call sites keep compiling and get `--help`
and proper "unknown long option" handling without an edit (2.3). `shell_getopt_r` gets the same with an `_r` twin. State
stays in `struct optstate`; new fields are two pointers' worth (`const char* lname` for messages, `int err`).

### 2.2 The long-option table is one string, not a struct array

Alternatives considered:

| form | per option | readability | verdict |
|---|---|---|---|
| GNU `struct option {name, has_arg, flag, val}` array, NULL-terminated | 24 bytes of data **plus** relocations in a PIE build, 4 tokens to write | familiar | too heavy: 30 builtins x 8 options = about 6 KB of data and relocations |
| a string, `"name=c name:=c name=c"`, parsed at run time | name length + 3 bytes, no relocations | one line per builtin, next to the optstring | **chosen** |
| long names inside `optstring` (`"R|recursive"`) | smallest | optstring is parsed on every call, and `:`/`+`/`|` collide | rejected |

Syntax of the string (space-separated words):

```
name=X      flag, returns X                  --verbose=v      (no argument)
name:=X     required argument in optarg      --relative-to:=R
name?=X     optional argument (only --name=value, never --name value)    --iso-8601?=I
```

`X` is **any single character the author picks**. If it also appears in `optstring`, the short option is a real alias and
`-X` works; if it does not, the option is long-only and `X` is only the `case` label. That is the whole mechanism: no
enum, no second numbering, and the `switch (c)` in the builtin stays the single place where options are handled. Several
words may map to the same `X` (`recursive=R recursive-dirs=R`, GNU synonyms `silent=q quiet=q`).

```c
/* cp: -R -r -f -i -p -n -v -a -t */
while((c = shell_getopt_long(argc, argv, "RrHLPfipnvat:T",
                             "recursive=R force=f interactive=i preserve=p no-clobber=n verbose=v archive=a "
                             "target-directory:=t no-target-directory=T dereference=L no-dereference=P")) > 0)
  switch(c) { case 'R': case 'r': ... }
```

Rules (each answers a GNU feature we deliberately do not copy):

- **Exact match only, no abbreviations.** `--rec` is an error. Reason: an abbreviation that is unique today becomes
  ambiguous the day an option is added, which breaks scripts silently; BusyBox and the BSDs do not abbreviate either.
- **No argument permutation (decided).** Parsing stops at the first operand, like the existing parser and like POSIX (`ls dir -l`
  treats `-l` as an operand). That is also what keeps `timeout 5 cmd --flag` and `env cmd --help` unambiguous.
- `--` ends options (already true). `--name=value` and, for required arguments, `--name value`. `--name=value` on a flag is an
  error (`option '--force' doesn't take an argument`).
- Short-option clusters, `-ovalue`, `-o value` stay exactly as they are.
- A long name the table does not know is an error even when a builtin has no table at all.
- The table is read at run time, so **no allocation, no relocation, no per-option code**. Cost estimate: the long path in
  the parser is about 70 lines of C, roughly 700 bytes of text; the strings total about 4-5 KB for the plan in section 4.
  **Measure with `size` on a `MinSizeRel` build before and after the parser, and again after each phase** (design metric 1),
  and gate the strings behind one macro so a size-critical build can drop them:
  `#if SHELL_LONGOPTS ... #define SHELL_LONG(s) s #else #define SHELL_LONG(s) NULL #endif` (the parser and `--help`
  stay; only the strings go). Decide the default after the measurement.

### 2.3 `--help` without a table entry

Behaviour, implemented once:

1. The parser recognises the exact word `--help` before it consults the table and returns one reserved code,
   `SHELL_OPT_HELP`. **`-h` is not special (decided):** a builtin without `h` keeps saying "invalid option" for it, and the
   builtins that own `h` (`grep`, `nl`, `set`, `sh`) are not affected at all. That also removes the "is `h` in the option
   string" check from the parser.
2. A table that defines its own `help=X` wins (nobody needs it, but nothing special is required to allow it).
3. **`builtin_invopt(argv)` does the work.** It already runs from every builtin's `default:` branch. New behaviour:
   - code is `SHELL_OPT_HELP`: look the builtin up with `builtin_search(argv[0])`, print `.args` then `.help` to stdout
     (exactly what `help <name>` prints, via one function extracted from `builtin_help.c`), **return 0**;
   - any other bad option: print the existing one-line message (`ls: -Z: invalid option`, and `ls: --foo: invalid option`
     for long ones; missing argument and "doesn't take an argument" get their own wording), then `usage: <args>` taken from
     `.args` on stderr (BSD style, one line, costs nothing per builtin), **return 2**.
4. Callers change from `default: builtin_invopt(argv); return 1;` to `default: return builtin_invopt(argv);`. That is a
   mechanical edit of 47 lines. Without it `--help` would exit 1 and a bad option would still exit 1. The sites that already
   return something else keep it: `env` 125 (the `env(1)` convention for "env itself failed"), `awk` 2, `dirstack` 2.
5. `filter_init()` handles the code itself before it calls `ops->option()`, because filters report errors through
   `ops->option(ctx, ch) < 0`: on `SHELL_OPT_HELP` it prints the help and returns a new "handled, status 0" value; a bad
   option returns status 2 (today it can only return -1 or 0, and the caller maps -1 to 1).

Which builtins get `--help`: **exactly those that call `shell_getopt`**, so nothing is listed anywhere. Builtins that take
arbitrary operands and never call it stay untouched on purpose: `echo`, `printf`, `test`/`[`, `expr`, `true`, `false`, `:`,
`eval`, `.`/`source`, `shift`, `break`, `continue`, `return`, `exit`, `trap`, `wait`, `kill` (`kill -l`, `kill -- -pid`),
`alias`/`unalias` operands, and `find` (own expression parser; add a one-line `--help` check there). `set` and `sh` parse
their own `+`/`-` option letters with `shell_getopt_r` and a `+` prefix: `set --help` must stay "set the positional
parameters", so `optstring` gets a leading `!` ("no help") handled by the parser, and `sh --help` is answered by `sh_main`
directly. Checked on the current tree: `env`, `command`, `exec` and `xargs` already pass `echo --help` through to the command,
and `timeout 5 echo --help` runs the command word as well; a test pins it (section 6).

Optional, not planned: `--version`. A fixed "shish builtin" string is noise; skip it unless a script is found that needs it.

### 2.4 Error wording and exit status

| case | stderr | status |
|---|---|---|
| unknown short option | `ls: -Z: invalid option` (unchanged) + `usage: ...` | **2** |
| unknown long option | `ls: --foo: invalid option` + usage | **2** |
| missing required argument | `cp: -t: option requires an argument` / `cp: --target-directory: option requires an argument` | **2** |
| argument given to a flag | `rm: --force: option doesn't take an argument` | **2** |
| `--help` | help on stdout | 0 |

Status 2 is "usage error", status 1 stays "the operation failed" (file missing, permission denied, no match for `grep`).
It was 1 everywhere before, so the step that changes `builtin_invopt` also updates every test that checks `$?` after a
bad option (found so far: `builtin-xargs.sh:204`; run `grep -rn 'invalid option' tests` and the `ctest` failure diff to
find the rest). The shell's own `sh` invalid-option exit is not part of this.

### 2.5 `touch`, `timeout`, `realpath`: delete the hand-rolled parsers

`extract_longopt()` exists twice and is inlined a third time, about 30 lines each. Moving them to the table removes about
90 lines, fixes the bug that a long option anywhere in `argv` (even after the command word of `timeout`) is pulled out
by position, and gives `--name value` and `--name=value` the same code path. This is the first conversion because it is
net negative in lines and has the existing tests (`builtin-touch.sh`, `builtin-timeout.sh`, `builtin-realpath.sh`).

---

## 3. `date`: the long options that need real work

`date` is the one builtin where long options are not aliases but features.

| option | meaning | plan |
|---|---|---|
| `-u`, `--utc`, `--universal` | UTC | alias |
| `-d STRING`, `--date=STRING` | show STRING instead of now | needs a parser, below |
| `-r FILE`, `--reference=FILE` | mtime of FILE | `stat`, trivial |
| `-I[FMT]`, `--iso-8601[=FMT]` | `date`, `hours`, `minutes`, `seconds`, `ns` | pure formatting, `YYYY-MM-DDTHH:MM:SS+hh:mm` |
| `--rfc-email` (`-R`), `--rfc-3339=FMT` | formatting | pure formatting |
| `-s`, `--set` | set the clock | stays unsupported (help says so) |
| `+FORMAT` | strftime | exists |

**Date string parser.** GNU's grammar is huge ("last friday 3pm", "2 weeks ago"). Scripts use a small subset; implement
exactly this and document it in the help text, error otherwise (`date: invalid date 'xyz'`):

1. `@SECONDS` (also negative), `now`, `today`, `yesterday`, `tomorrow`
2. `YYYY-MM-DD`, `YYYY-MM-DD[T ]hh:mm[:ss]`, optional `Z` or `+hh:mm` / `-hh:mm`; `hh:mm[:ss]` alone means today
3. a relative tail: `[+-]N unit [ago]` with units `second minute hour day week month year` (plural accepted), repeatable
   (`2 days ago`, `+1 month`, `yesterday 12:00`)

`touch -d/--date` has the first two forms in `touch_parse_date()`. **Extract that into one shared parser** (new
`lib/date/date_parse.c`, one function per file, `strview`/`(p, n)` in, `time_t` out, no `<stdio.h>`) and let `touch` and `date`
both call it; `touch` gains the relative tail for free. This is the only new library code in the whole plan. Tests: a table
of strings with a fixed `TZ=UTC` and a fixed "now" (`date -d '@0'` style) so the output is deterministic.

---

## 4. Which builtins get which long options

Names are the GNU names, so GNU scripts run unchanged. Only options that the builtin **already implements or is about to
implement in the same change** are listed; no long name for a feature that does not exist. `-` marks "no long form in GNU".
Order is by how often a script uses it, which is also the order of the phases.

### Phase 1 (the three ad-hoc parsers, `date`, the file utilities)

| builtin | table |
|---|---|
| `realpath` | `relative-to:=R relative-base:=B canonicalize-existing=e canonicalize-missing=m logical=L physical=P quiet=q strip=s no-symlinks=s` |
| `touch` | `date:=d reference:=r time:=T no-create=c` (`--time=access\|atime\|use`, `modify\|mtime` map to `-a`/`-m` in the builtin) |
| `timeout` | `signal:=s kill-after:=k foreground=f preserve-status=p verbose=v` |
| `date` | section 3 |
| `cp` | `recursive=R force=f interactive=i preserve=p no-clobber=n verbose=v archive=a dereference=L no-dereference=P target-directory:=t no-target-directory=T` |
| `mv` | `force=f interactive=i no-clobber=n verbose=v target-directory:=t no-target-directory=T update=u` |
| `rm` | `force=f interactive=i recursive=r dir=d verbose=v` |
| `ln` | `symbolic=s force=f verbose=v logical=L physical=P no-dereference=n` |
| `mkdir` | `parents=p mode:=m verbose=v` |
| `rmdir` | `parents=p verbose=v ignore-fail-on-non-empty=f` |
| `ls` | `all=a almost-all=A directory=d classify=F recursive=R reverse=r inode=i dereference=L sort:=S color?=C` (`--sort=size\|time\|none` selects `-S -t -f`; `--color`: section 4.1) |
| `mktemp` | `directory=d quiet=q tmpdir?=p dry-run=u suffix:=X` |
| `readlink` | `canonicalize=f canonicalize-existing=e canonicalize-missing=m no-newline=n` |
| `basename` | `multiple=a suffix:=s` |
| `chmod` | `recursive=R verbose=v changes=c silent=f quiet=f reference:=F` |

`ls` has no `-h` today and gets none here; `--human-readable` is dropped from the table until sizes are shown that way.

### Phase 2 (the filters and text utilities, through `ops->opts`)

The 13 filters share `struct filter_ops`; add one member `const char* longopts` next to `opts`, and `filter_init()` passes it to
`shell_getopt_long`. That is the only change outside each filter's own table.

| builtin | table |
|---|---|
| `grep` | `ignore-case=i invert-match=v count=c line-number=n files-with-matches=l files-without-match=L quiet=q silent=q recursive=r extended-regexp=E fixed-strings=F basic-regexp=G regexp:=e file:=f word-regexp=w line-regexp=x with-filename=H no-filename=h only-matching=o no-messages=s max-count:=m after-context:=A before-context:=B context:=C null-data=z` |
| `sed` | `quiet=n silent=n expression:=e file:=f regexp-extended=E regexp-extended=r in-place?=i separate=s null-data=z` |
| `sort` | `reverse=r numeric-sort=n unique=u key:=k field-separator:=t output:=o ignore-case=f ignore-leading-blanks=b stable=s check=c merge=m` |
| `uniq` | `count=c repeated=d unique=u ignore-case=i skip-fields:=f skip-chars:=s check-chars:=w` |
| `head`, `tail` | `lines:=n bytes:=c quiet=q silent=q verbose=v` (`tail`: `follow?=f retry=F`) |
| `cut` | `bytes:=b characters:=c fields:=f delimiter:=d only-delimited=s complement=C output-delimiter:=O` |
| `tr` | `delete=d squeeze-repeats=s complement=c truncate-set1=t` |
| `cat` | `number=n number-nonblank=b squeeze-blank=s show-all=A show-ends=E show-tabs=T show-nonprinting=v` |
| `wc` | `bytes=c chars=m lines=l max-line-length=L words=w` |
| `tee` | `append=a ignore-interrupts=i` |
| `nl`, `paste`, `compress`, `uncompress` | `stdout=c decompress=d force=f keep=k test=t` for the compressors; `nl`/`paste` as their GNU names once someone needs them |

### Phase 3 (system utilities)

| builtin | table |
|---|---|
| `xargs` | `null=0 arg-file:=a delimiter:=d eof?=E replace?=I max-lines:=L max-args:=n max-procs:=P interactive=p no-run-if-empty=r max-chars:=s verbose=t exit=x open-tty=o` |
| `env` | `ignore-environment=i unset:=u chdir:=C null=0` |
| `id` | `user=u group=g groups=G name=n real=r` |
| `uname` | `all=a kernel-name=s nodename=n kernel-release=r kernel-version=v machine=m processor=p operating-system=o` |
| `split` | `lines:=l bytes:=b suffix-length:=a numeric-suffixes=d` |
| `digest` | `check=c binary=b text=t quiet=q status=s tag=T` |
| `which`, `hostname` | `all=a`; `fqdn=f` |

`cd`, `pwd`, `type`, `command`, `read`, `export`, `readonly`, `unset`, `umask`, `ulimit`, `alias`, `jobs`, `trap`, `getopts`, `set`:
**no long options** (POSIX special and regular shell builtins have none, and the bash ones that do (`read`, `ulimit`, `set -o`)
have a different style). They still get `--help` where they call the parser.

`find` keeps its own single-dash primaries (`-name`, `-type`); it only gains `--help`.

### 4.1 `ls --color[=yes|no|auto]` (minimal)

Fixed colours, no `LS_COLORS`/`dircolors`, no per-extension rules. About 25 lines in `builtin_ls.c`.

| argument | meaning |
|---|---|
| `yes` (also `always`, `force`) or no argument | colour on |
| `no` (also `never`, `none`) | colour off (the default when the option is absent) |
| `auto` (also `tty`, `if-tty`) | on when stdout is a terminal, `TERM` is not `dumb`, and `NO_COLOR` is empty or unset |
| anything else | usage error (status 2), `ls: --color: invalid argument 'x'` |

The GNU spellings in brackets cost one `str_equal` each and are what existing scripts and aliases pass, so they are accepted;
the shish spelling is the documented one. `--color` with no argument means `yes`, as in GNU.

Colours (SGR, one table, nothing configurable):

| entry | code | colour |
|---|---|---|
| directory | `94` | light blue |
| symbolic link | `36` | cyan |
| regular file with an execute bit | `33` | yellow |
| FIFO, socket | `35` | magenta |
| block or character device | `93` | light yellow |
| everything else | none | plain |

Implementation, keeping the diff small:

- One flag `unsigned color : 1;` in `struct ls_opts`, set once after option parsing from the argument and, for `auto`,
  `isatty(fd_out->fd)` plus the two environment checks.
- One function `ls_put_name(buffer* b, const char* name, unsigned int mode)` that writes `ESC[<code>m`, the name, `ESC[0m`
  when colour is on and `mode` has a code, and just the name otherwise. It also takes over the `-q` quoting loop now inline in
  `ls_put()`, so there is one place that prints a name. The code is picked by a six-line `if` chain on `mode`; no table, no
  allocation.
- It replaces the two places that print a name: `ls_put()` (long and one-per-line formats) and the column printer
  (`buffer_puts(fd_out->w, v[i].name)`).
- **Column widths do not change**: `ls_cell_len()` and the column-width code measure `str_len(name)` and the `-F` marker, never
  the bytes written, so the escape sequences cannot misalign columns. The `-F`/`-p` marker is printed after the reset sequence.
- In `-l` format the symlink target after `->` stays uncoloured (GNU colours it; not worth the lines).
- `lstat` data is already in `ls_ent`, so no extra system calls.

Test: `ls --color=yes` on a temp tree with a directory, a symlink, an executable and a plain file, checked byte for byte
(the escape bytes through `printf`); `--color=no` and absent equal the output without escapes; `--color=auto` into a pipe has
none; `--color=bogus` exits 2; `-F --color=yes` and `-C --color=yes` keep their column layout (compare against
`--color=no` after stripping the escapes with `sed`).

### Documentation of the options

The `.help` strings stay hand-written (a generated table would cost more code than the text it saves), but each option line
lists both spellings: `-R, -r, --recursive`. A test checks that every long name in a builtin's table appears in
`help <name>` (section 6), so the text cannot drift from the table.

---

## 5. Order of work

Each step builds, passes `ctest` as before (14 of 190 failing before and after; compare the failure lines, not only the
count) and has its own commit.

1. **Parser.** `shell_getopt_long` + `SHELL_OPT_HELP` + `struct optstate` fields + `!` prefix. Unit test in
   `tests/lib/` (no builtins involved). `shell_getopt` forwards to it.
2. **`builtin_invopt` + the 47 call sites + `filter_init`.** Extract the print from `builtin_help.c`, return 2 for a bad
   option, and fix the tests that expected 1. Now every builtin answers `--help`, and `ls --help` stops printing
   `ls: --: invalid option`. Measure size.
3. **`realpath`, `touch`, `timeout`** onto the table; delete the three `extract_longopt` copies.
4. **Shared date parser** (`lib/date/`), `touch` on it, then `date` with section 3.
5. **Phase 1 tables** (with `ls --color` as its own commit, section 4.1), one builtin per commit, each with its `tests/builtin-<name>.sh` extended: for every pair
   `--long` / `-s` an equality test on a temp tree (`cp --recursive` equals `cp -R`).
6. **Phase 2** (filters), **Phase 3**.
7. **Measure** `size` on `MinSizeRel` after steps 2, 5, 6, 7 and put the numbers in this file; decide the `SHELL_LONGOPTS` default.

Estimated net effect on source: parser +70 lines, `builtin_invopt` +25, `extract_longopt` copies -90, date parser moved (not
added) plus about 60 new for the relative tail, 47 one-line call-site edits, about 45 table lines. Verify the line counts at the end.

## 6. Tests

- `tests/lib/getopt_test.c` (wired like `path_test`): short clusters, `-ovalue`, `--name=value`, `--name value`,
  `--flag=value` error, missing argument for short and long, `--` and "stop at first operand", optional argument, long-only
  code, a code shared by two names, `--help` with and without `h` in `optstring` (`-h` must stay untouched in both), unknown
  long option, `!` prefix, restart on a second call.
- `tests/builtin-help.sh`: loop over every name in `help`'s listing. For each builtin that is **not** in an explicit
  allow-list of operand-style builtins (section 2.3): `$b --help` exits 0 and prints exactly what `help $b` prints;
  `$b --no-such-option` exits 2, writes the message and a `usage:` line to stderr and nothing to stdout; `$b -h` is *not* the
  help (it is whatever the builtin does with `-h`, or an invalid option).
- `tests/builtin-longopts.sh`: for each table row of section 4, `X --long` and `X -s` give the same result on a temp tree
  (output and exit status); plus the "option text appears in `help <name>`" check.
- `timeout 5 sleep --help`-style cases: the command word's own `--help` must reach the command (`timeout`, `env`,
  `xargs`, `command`, `exec`, `nice`-like), pinned in `tests/fixed.sh` together with the `ls --help` fix.
- `BUGS`: record `ls --help` printing `ls: --: invalid option` now, and remove the entry with step 2.

## 7. Decisions

Answered (2026-10-14):

1. **Argument permutation: no.** Parsing stops at the first operand.
2. **Abbreviations: no** (exact names only), as planned.
3. **Exit status for a bad option: 2.** Operation failures stay 1; `env` keeps 125.
4. **`-h`: never help.** Only `--help` does. A builtin that owns `h` keeps it; a builtin that does not says "invalid option".
5. **`ls --color[=yes|no|auto]`: yes, minimal** (section 4.1).
6. **Table spelling:** the string form `name:=X` / `name?=X`, as chosen in 2.2.

Still open:

7. **`--version`:** not planned. Say if a script needs it.
8. **`auto` and the environment:** the plan honours `NO_COLOR` and `TERM=dumb` (two lines). Drop them if you want `auto` to mean
   "stdout is a terminal" and nothing else.
9. **Colours for the "etc.":** FIFO/socket magenta and devices light yellow are my pick; the three you named (directories light
   blue, symlinks cyan, executables yellow) are as stated.
