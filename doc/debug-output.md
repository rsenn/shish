# Debug output in shish

Inventory of the existing debug-print code (what is there, where, what it prints, and
when), the problems found while exercising it, and a plan for a new, uniform
"trace the evaluator" layer.

Everything in part 1 was checked against a real build (see [How to get debug output](#how-to-get-debug-output)).
Line numbers are for the tree at the time of writing (`main`, after the `filter-chain` merge)
and will drift; the function name is the stable handle.

- [1. Existing debug output](#1-existing-debug-output)
- [2. Problems found](#2-problems-found)
- [3. Design for the new debug output](#3-design-for-the-new-debug-output)
- [4. Checkpoints to instrument](#4-checkpoints-to-instrument)
- [5. Rollout order](#5-rollout-order)

---

## 1. Existing debug output

### How to get debug output

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug \
  -DDEBUG_OUTPUT=ON -DDEBUG_FD=ON -DDEBUG_FDSTACK=ON -DDEBUG_FDTABLE=ON \
  -DDEBUG_PARSE=ON -DDEBUG_JOB=ON -DDEBUG_BUILTIN=ON \
  -DCMAKE_C_FLAGS="-include $PWD/lib/uint64.h"     # work-around, see problem P1
cmake --build build/debug -j --target shish
cd /some/scratch/dir && /path/to/build/debug/shish -x -c 'echo a | cat'
less debug.log
```

| thing | detail |
|---|---|
| master switch | `DEBUG_OUTPUT` (`CMakeLists.txt:152`; a `*Deb*` build type flips it on by default, `cmake/Builtins.cmake:85-91`). Adds `src/debug/*.c` to the library (`CMakeLists.txt:209`). |
| module switches | `DEBUG_FD`, `DEBUG_FDSTACK`, `DEBUG_FDTABLE` (+ `DEBUG_ALLOC`, `DEBUG_PARSE`) via `debug_module_options` (`CMakeLists.txt:39`); `DEBUG_PARSE`, `DEBUG_JOB`, `DEBUG_BUILTIN` via `debug_flag` (`CMakeLists.txt:154-156`). Each is a plain `-DDEBUG_<M>`; a site needs **both** `DEBUG_OUTPUT` and its module flag. |
| colour | `DEBUG_COLOR` → `COLOR_DEBUG` (`src/debug.h:28`); most files include `debug.h` with colours on, `src/debug/debug_begin.c:1` forces `DEBUG_NOCOLOR`. |
| destination | **`debug.log` in the current directory**, truncated by the first `debug_open()` (`src/debug.h:165`). `debug_output` points at `debug_buffer` (`src/debug/debug_begin.c:5-6`). |
| exceptions | `builtin_trap.c` redirects to **stderr** (`debug_to(buffer_2)`), because it runs from signal-ish context. |
| runtime gate | three sites additionally require `set -x`/`-x` (`sh->opts.xtrace`): `fd_setfd.c:52`, `parse_gettok.c:38`, `parse_simple_command.c:130`. |
| dump builtin | `dump -t/-s/-f/-j` (fdtable / fdstack / fd list / jobs) exist only when the matching `DEBUG_*` is set (`src/builtin/builtin_dump.c:35-44`, `:68-89`, `builtin_table.c:243-252`); `dump -v/-l/-F` (vartab / functions) always. It also needs `BUILTIN_DUMP` enabled. |
| `-I` option | `sh_main.c:276`: `#ifdef _DEBUG` adds a `-I` (`no_interactive`) shell option. Unrelated to `DEBUG_OUTPUT`; `_DEBUG` is defined by `BUILD_DEBUG`. |

### Print helpers (`src/debug.h`, `src/debug/*.c`)

Compiled only with `DEBUG_OUTPUT` (or `SHPARSE2AST`, which reuses them for the AST dumper).

| macro / function | does |
|---|---|
| `debug_s/n/xn/c/b/ws/nl/fl/nl_fl` (`debug.h:102-115`) | thin wrappers over `buffer_put*` on `debug_output`; no-ops without `DEBUG_OUTPUT` |
| `debug_fn/_ws/_nl/_nf` | print `__func__()` (+ space / newline / newline+flush) |
| `debug_to(buf)` | redirect `debug_output` (used by `builtin_trap.c`) |
| `debug_open()` (`debug.h:165`) | open+truncate `debug.log` once |
| `debug_indent/newline/nindent` | indentation by `depth * debug_nindent` |
| `debug_begin/end(s, depth)` | open/close a `[ … ]` block |
| `debug_node/list/sublist/subnode(…)` | dump an AST node / sibling list as pretty JSON-like text |
| `debug_str/stralloc/unquoted/squoted/char/ulong/xlong/ptr/range/position/location/subst/redir/argv/space` | leaf printers: `"key": value` lines |
| `debug_emit_loc`, `debug_emit_range` | which position info `debug_node` includes (`debug.h:22-23`) |
| `dump_flags(buf, bits, names[], pad)` / `debug_flags` | render a bit set as `A|B|C` |

### Index of active sites

"Gate" is the full preprocessor condition (`OUT` = `DEBUG_OUTPUT`). "When" is the shell
task that is running when the line is produced. Samples are real output, trimmed.

#### Parser (`DEBUG_PARSE`)

| file:line | gate | what it prints | when |
|---|---|---|---|
| `src/parse/parse_getarg.c:17` | OUT+PARSE | `parse_getarg <word>` – each word as the parser finishes it | parsing a simple command's arguments |
| `src/parse/parse_gettok.c:37` | OUT+PARSE, **+ `-x`** | `parse_dump <tok flags> <tok>` – lexer state after each token (via `parse_dump()`, `src/parse/parse_dump.c`) | tokenizing; only with `-x` |
| `src/parse/parse_simple_command.c:129` | OUT+PARSE, **+ `-x`** | `parse_simple_command loc = "file:l:c" «text»` | a simple command was completed |
| `src/parse/parse_command.c:101` | OUT+PARSE | `parse_command command = { …JSON tree… }` | a whole command was parsed |
| `src/parse/parse_list.c:57` | OUT+PARSE (not `SHPARSE2AST`) | `parse_list [N] cmds = [ … ]`, only for lists with >1 command | a `;`/`&&`/newline list was completed |
| `src/parse/parse_grouping.c:39` | OUT+PARSE | `parse_grouping grouping = { … }` | `{ …; }` / `( … )` completed |
| `src/parse/parse_function.c:67` | OUT+PARSE (not `SHPARSE2AST`) | `parse_function node = { … }` | `name() { … }` completed |
| `src/parse/parse_arith.c:22` | OUT+PARSE | `parse_arith tree = { … }` | `$(( … ))` expression parsed |
| `src/parse/parse_expect.c:13` | OUT+PARSE | (bare) `debug_list(nfree)` – the partially built tree being thrown away | a syntax error while a subtree is half built |
| `src/sh/sh_loop.c:53` | OUT+PARSE (not `SHPARSE2AST`) | `sh_loop list = [ … ]` – the tree about to be evaluated | main read-parse-eval loop, once per parsed list |

```text
parse_getarg echo
parse_getarg hi
parse_command command = {
  "kind": "simple_command",
  "bgnd": 0,
  "args": [
    { "kind": "word", "list": [ { "kind": "string", "flag": "0x0", "loc": "<string>:1:1", "stra": "echo" } ] },
    …
sh_loop list = [ { "kind": "simple_command", … } ]
```
With `-x` additionally:
```text
parse_dump NAME
parse_dump (P_NOKEYWD P_NOASSIGN) NAME
parse_dump (P_NOKEYWD P_NOASSIGN) |
parse_simple_command loc = "<string>:1:1" «echo a»
```
(`debug_node` pretty-prints one key per line; the samples above are folded for the page.)

#### File descriptors (`DEBUG_FD`, `DEBUG_FDSTACK`, `DEBUG_FDTABLE`)

| file:line | gate | what it prints | when |
|---|---|---|---|
| `src/fdstack/fdstack_link.c:10` | OUT+FDSTACK | `fdstack_link n=<fd>` | a struct fd is linked into the current fdstack level (startup: `n=0,1,2,-1`; then every redirection / pipe / subst) |
| `src/fdstack/fdstack_pipe.c:86` | OUT+FDSTACK | `fdstack_pipe n=<count> fds=<addr>` | `$(…)` / here-doc fds are being converted to real pipes just before a fork (`exec_program`, `eval_pipeline`, `builtin_xargs`) |
| `src/fdstack/fdstack_data.c:26` | OUT+FDSTACK | `fdstack_data` + `fd_dump()` of the subst fd | parent draining a child's output into the `$(…)` buffer |
| `src/fd/fd_pipe.c:42` | OUT+FD | `fd_pipe n=<vfd> e=<real> ret=<other end>` | `pipe()` created for a struct fd |
| `src/fd/fd_setfd.c:51` | OUT+FD, **+ `-x`** | `fd_setfd #<n> e=<real> mode=FD_READ\|FD_WRITE` | a struct fd is bound to a real descriptor |
| `src/fd/fd_pop.c:15` | OUT+FD | `fd_pop 0x<addr>` | a struct fd leaves the stack (end of a redirection scope, a pipeline stage, …) |
| `src/fd/fd_close.c:61,71` | OUT+FD | `fd_close #<real fd>` (read side, then write side) | the real descriptor behind a struct fd is closed |
| `src/fdtable/fdtable_dup.c:86` | OUT+FDTABLE | `fdtable_dup #<old> = <new>` | `dup()`/`dup2()` performed on the table |
| `src/fdtable/fdtable_resolve.c:113` | OUT+FDTABLE | `fdtable_resolve(<fd_dump line>, MOVE\|FORCE…) = DONE\|ERROR\|PENDING` | a pending redirection is turned into real fd state |
| `src/exec/exec_program.c:67` | OUT+FDTABLE | full `fdtable_dump()` (table below) | just before forking an external program |

```text
fdstack_link n=1
fdstack_link n=3
fd_pipe n=1 e=5 ret=4
fdstack_pipe n=1 fds=5acf5efa02a0
  fd name             level  e  mode                      buffer(s)
------------------------------------------------------------------------------------------------------------
  -1 <string>           0   -1  STRING
------------------------------------------------------------------------------------------------------------
   0 pipe               1    4  READ|PIPE|TMPBUF          r=[ p=0, n=0, a=1024, x@p="", fd=4, op=<read> ]
   1 file               0    1  WRITE|FILE|TMPBUF         w=[ p=0, n=0, a=1024, x@p="", fd=1, op=<write> ]
fd_pop 0x00007ffdb450f870
fdtable_dup #4 = 0
fdtable_resolve(   0 pipe               1    0  READ|PIPE|TMPBUF   r=[ … fd=0 … ], MOVE) = DONE
fd_close #5
```
The `ESC[59G` that shows up before a `w=[` continuation is `fd_dump()` positioning the cursor;
it is harmless in a terminal and noise in a file.

#### Dump routines (called by the sites above and by `dump`)

| file:line | gate | prints |
|---|---|---|
| `src/fd/fd_dump.c:3` | OUT | one `struct fd` row: vfd, name, level, real fd, mode flags, `r=[…]`/`w=[…]` buffer state |
| `src/fd/fd_dumplist.c:1` | OUT+(FD\|FDSTACK\|FDTABLE) | header + `fd_dump()` for each `fd_list[]` slot (real-fd view) |
| `src/fdtable/fdtable_dump.c:1` | OUT+(FD\|FDSTACK\|FDTABLE) | per virtual fd, the whole shadow chain (`fd->parent`) |
| `src/fdstack/fdstack_dump.c:1` | OUT | every fd of every fdstack level, innermost first |
| `src/job/job_dump.c:1` | OUT+JOB | job table: id, pgrp, command, done, pids |

#### Builtins, expansion, misc

| file:line | gate | what it prints | when |
|---|---|---|---|
| `src/builtin/builtin_trap.c:104` | OUT+BUILTIN | `trap handler <sig>` (**stderr**) | a trapped signal fires |
| `src/builtin/builtin_trap.c:328` | OUT+BUILTIN | `trap_uninstall <sig>` (**stderr**) | trap removed/reset |
| `src/builtin/builtin_trap.c:607` | OUT+BUILTIN | `builtin_trap <sig>` + `"code": …` (**stderr**) | `trap 'code' SIG` executed |
| `src/builtin/extra/builtin_expr.c:187` | OUT | `debug_list(expr)` – the parsed `expr` tree | `expr` builtin, after parsing its argv |
| `src/expand/expand_arith_expr.c:128` | OUT | `expand_arith_expr <node>` | evaluating `$(( … ))` |
| `src/eval/eval_pipeline.c:643` | OUT | `"forked": <pid>` (no newline; interleaves with the next line, see P4) | a pipeline stage was forked |
| `src/sh/sh_init.c:33` | OUT | (nothing printed) opens `debug.log` | shell start-up |
| `src/sh/sh_fmt.c:119` | OUT | `"tree_columnwrap": N` / `"indent_width": N` | `shformat` start-up |

```text
builtin_trap 2
"code": echo caught
trap_uninstall 2
```

### Disabled relics (`DEBUG_OUTPUT_`, trailing underscore, or `#if 0`)

These never compile. They document places where someone once wanted output; several are
good candidates for the new layer (part 4).

| file:line | would print | task |
|---|---|---|
| `src/expand/expand_args.c:34` | `debug_node(arg)` per argument | word expansion of a command's args |
| `src/eval/eval_simple_command.c:82` | `Vars <list>` / `Assigns` | prefix assignments (`X=1 cmd`) |
| `src/eval/eval_simple_command.c:225` | `Redirection <node> fd { n= }` | each redirection of a simple command |
| `src/vartab/vartab_add.c:47,54` | `assert(dist != 0)`: variable must not already exist | inserting a variable into a scope |
| `src/parse/parse_error.c:43` (`#if 0`) | the tree/node being parsed at the syntax error | parse error report |
| `src/term/term_read.c:152,196` | each input char, and the return value | interactive line editing |
| `src/term/term_newline.c:12` | function name | interactive line editing |
| `src/prompt/prompt_expand.c:24,33`, `prompt_parse.c:63` | prompt node / expanded prompt | PS1/PS2 expansion |
| `src/prompt/prompt_nextline.c:6`, `prompt_reset.c:7` | function name + prompt number | PS1/PS2 selection |
| `src/source/source_skip.c:15` | each skipped char | input reader |

---

## 2. Problems found

| id | problem | evidence / fix |
|---|---|---|
| P1 | **FIXED** (`lib/buffer.h` now includes `uint64.h`). **`DEBUG_OUTPUT` did not build.** `debug.h` uses `buffer_putlonglong` and `buffer_putxlonglong`, declared in `lib/buffer.h:153-158` only `#ifdef UINT64_H`. `src/builtin/builtin_error.c` (and 11 other files) include `buffer.h` before `uint64.h`, so the declaration is skipped: `implicit declaration of function 'buffer_putlonglong'`. | Work-around: `-include lib/uint64.h`. Real fix: `#include "uint64.h"` at the top of `lib/buffer.h`, or drop the `#ifdef`. |
| P2 | Output is a **single file, `debug.log`, in the cwd**, truncated by whichever process calls `debug_open()` first; forked children share the fd and its offset, so pipelines/`$(…)` interleave arbitrarily. | New layer: `O_APPEND`, one `write()` per event, pid in every line. |
| P3 | Most modules have **one** print (or none) at the *end* of a step: you see results, not causes. Nothing at all in `eval_*`, `redir_*`, `exec_command`, `var_*`, `vartab_*`, `sh_push/pop`, `job_*`, signals. | part 4 |
| P4 | **FIXED** (now `eval.pipeline.fork(pid=…)`). `"forked": <pid>` had no newline (`eval_pipeline.c:644`), so the next event is glued to it (`"forked": 3656924fd_pop 0x…`). | new format is line-atomic. |
| P5 | `DEBUG_ALLOC` is a CMake option with **no consumer** anywhere; `DEBUG_JOB` and `DEBUG_BUILTIN` only guard `job_dump` and `builtin_trap.c`. | wire up or drop the flags. |
| P6 | Three sites need a runtime `set -x` *and* the compile flag; the rest ignore `-x`. Inconsistent. | new layer: one runtime selector (below). |
| P7 | `builtin_trap.c` prints to stderr via `debug_to(buffer_2)` then resets it to `&debug_buffer`; if `debug_buffer` had been `debug_to()`'d elsewhere the restore is wrong, and stderr output is mixed into the user's own stderr. | new layer: fixed destination, no global swap. |
| P8 | `fd_dump()` embeds cursor-movement escapes (`ESC[59G`) in the log. | drop in file output. |
| P9 | Style: hand-rolled `buffer_puts` sequences per site; no common prefix, no depth, no way to filter or grep by module. | new layer. |

---

## 3. Design for the new debug output

Goal: trace the **evaluator** – every time the shell environment changes, and at every step
on the way to running a builtin or forking/exec'ing a program – in a form that a human can
read like pseudo-code and a script can parse.

### 3.1 Event line grammar

One event per line, written atomically, so multiple processes can share the file.

```
line      = prefix SP body EOL
prefix    = "[" pid ":" depth "]"  SP  module "." event
body      = call | ret | note | struct
call      = name "(" [ arg { "," SP arg } ] ")"                  ; entering / performing an operation
ret       = "=>" SP value                                        ; result of the call on the previous line of same pid:depth
note      = "#" SP text                                          ; free comment
struct    = name SP "{" [ field { "," SP field } ] "}"           ; state snapshot, one line
arg,field = ident "=" value
value     = int | 0xHEX | string | list | struct | flags | enum | "NULL"
string    = '"' escaped '"'                                       ; \n \t \" \\ \xNN
list      = "[" [ value { "," SP value } ] "]"
flags     = ident { "|" ident }                                   ; FD_READ|FD_PIPE
enum      = ident
```

- `pid:depth` – `depth` is `eval_depth()` (`src/eval.h:94`), so nesting of `$(…)`, functions and
  subshells is visible.
- Lines that would be too long (whole AST nodes) keep using the existing multi-line
  `debug_node()` JSON block, introduced by a `struct` line ending in `{` and terminated by a
  matching `}` at column 0.
- Strings are always quoted and escaped, so `argv` can be round-tripped.

Sample of the target output for `X=1 cat <in | wc -l`:

```text
[812:0] eval.simple_command(loc="<string>:1:1", argc=1, nassign=1, nredir=1, bgnd=0, flags=E_EXIT)
[812:0] expand.args() => argv=["cat"]
[812:0] var.push(scope=prefix_assign, function=1) => depth=2
[812:0] var.set(name="X", value="1", flags=V_EXPORT|V_LOCAL)
[812:0] redir.eval(fd=0, op="<", target="in", flags=R_NOW)
[812:0] fd.open(fd=0, file="in", mode=FD_READ) => e=5
[812:0] exec.command(kind=H_PROGRAM, path="/bin/cat", argv=["cat"], flag=X_NOWAIT)
[812:0] exec.program.fork()
[812:0] fdstack.pipe(n=0)
[813:0] sh.forked() # child
[813:0] fdtable.exec { 0=file:5 1=pipe:4 2=tty } # what execve() will see
[813:0] exec.program.execve(path="/bin/cat", argv=["cat"], nenv=57)
[812:0] job.new(pid=813, cmd="cat") => id=1
```

### 3.2 API

New header `src/trace.h` (name is free; keeps `debug.h` untouched). All macros expand to
nothing unless `DEBUG_OUTPUT`, so release builds pay nothing.

```c
#define TRACE_MODULES(X) X(EVAL) X(EXPAND) X(REDIR) X(EXEC) X(FD) X(FDSTACK) \
                         X(FDTABLE) X(VAR) X(SH) X(JOB) X(SIG) X(BUILTIN) X(PARSE)

trace_on(MOD)                         /* runtime: is this module selected? */
TRACE_CALL(MOD, "event", "fmt", ...)  /* prints  [pid:depth] mod.event(fmt…)  */
TRACE_RET(MOD, "event", "fmt", ...)   /* prints  [pid:depth] mod.event => …   */
TRACE_NOTE(MOD, "fmt", ...)           /* prints  # …                          */
TRACE_STRUCT(MOD, "name", dumpfn, p)  /* prints  name { … } via a dump helper */
trace_argv(argv), trace_flags(bits, names), trace_str(s)   /* value formatters */
```

Selection and destination are **runtime**, compile-time `DEBUG_OUTPUT` just makes it
available – no rebuild to look at one module:

| variable | meaning |
|---|---|
| `SHISH_TRACE=eval,redir,exec,fd` | comma list of modules, `all`, or `-name` to exclude; unset = off |
| `SHISH_TRACE_FILE=path` | default `trace.log` (a different file from the legacy `debug.log`, which is truncated on open); opened `O_APPEND\|O_CREAT`, moved to fd >= 200 with `FD_CLOEXEC`, never truncated |
| `SHISH_TRACE_FILE=-` | stderr |

Each event is formatted into a per-process buffer and emitted with one `write()`; the
pid comes from `sh_pid`, so a `fork()` needs no special handling other than the child calling
`trace_reopen()` after `sh_forked()`.

Value dumpers to add next to the existing `fd_dump`/`fdtable_dump`/`fdstack_dump`/`job_dump`:
`trace_fd(struct fd*)`, `trace_cmd(struct command*)`, `trace_env(struct env*)`,
`trace_redir(struct nredir*)`, `trace_var(struct var*)`, `trace_sigset()` and
`trace_fdmap()` (real kernel view: `readlink /proc/self/fd/N` on Linux, `fcntl(F_GETFD)`
elsewhere). The existing `debug_node()` keeps producing the AST.

The old `DEBUG_<MODULE>` compile flags collapse into the runtime list above; the current
sites are ported one by one (part 5, step 1) and the flags removed.

### 3.3 What every event carries

Always: pid, depth, module, event. Where it applies: `loc="file:line:col"` of the node
being evaluated (from `union node`), the resolved `argv`, the exit status on `=>`,
and – for anything that changes state – both the *before* and *after* of the one thing that
changed (not a whole-world dump).

---

## 4. Checkpoints to instrument

Each row is one proposed trace point: where it goes, what it emits, and which question it
answers when a script misbehaves. All line numbers are call sites in the current tree.

### 4.1 Evaluation (`src/eval/`)

| where | event | payload | answers |
|---|---|---|---|
| `eval_node.c:9` | `eval.node` | node kind (`N_SIMPLECMD`, `N_IF`, …), `loc`, `e->flags` | what is the evaluator looking at; the entry point for every step |
| `eval_tree.c`, `eval_cmdlist.c` | `eval.list` | number of commands, `E_LIST` flags | list/`&&`/`||` short-circuit decisions |
| `eval_and_or.c`, `eval_if.c`, `eval_case.c`, `eval_loop.c`, `eval_for.c` | `eval.branch` | condition status, branch taken, loop iteration / `for` word | control-flow trace |
| `eval_push.c:12`, `eval_pop.c:10` | `eval.push` / `eval.pop` | `flags` (`E_ROOT`, `E_EXIT`, `E_PRINT`…), depth, return `exitcode` | frame nesting; who owns a `jump`/`return` |
| `eval_jump.c:9`, `eval_return.c:9`, `eval_exit.c:8` | `eval.jump/return/exit` | levels, `cont`, value | non-local exits (`break`, `continue`, `return`, `exit`) |
| `eval_command.c:20` (redir scope: `:33` push, `:43` `redir_eval`) | `eval.redir_scope` | number of redirections, `fdstack` level | when redirections become active/inactive |
| `eval_simple_command.c:38` entry | `eval.simple_command` | `loc`, counts of args/assigns/redirs, `bgnd` | one line per command executed |
| `eval_simple_command.c:51` (after `expand_args`) | `expand.args` | expanded `argv` | word expansion result (the relic at `expand_args.c:34` wanted this) |
| `eval_simple_command.c:80` (after `expand_vars`) | `expand.assigns` | `name=value` list | prefix assignments (relic `:82`) |
| `eval_simple_command.c:111` / `:322` | `var.push` / `var.pop` | scope kind (`prefix_assign`), function flag | temporary env for `X=1 cmd` |
| `eval_simple_command.c:209` | `redir.eval` (via 4.2) | one per redirection | relic `:225` |
| `eval_simple_command.c:300` (before `exec_command`) | `exec.command` | see 4.3 | the hand-off to execution |
| `eval_subshell.c:30,41,42,90` | `subshell.enter/leave` | `fdstack`/`vartab`/`sh` push+pop, `E_ROOT` | `( … )` never forks; shows what state is saved and restored |
| `eval_function.c:105` | `eval.function.define` | name | function definition |
| `eval_pipeline.c:405` | `eval.pipeline` | stage count, `bgnd`, mode (`no-fork filter chain` / `sequential` / `fork`) | which of the three pipeline strategies was picked and why |
| `eval_pipeline.c:294,385` | `pipeline.filter.enter/leave` | fdstack level | no-fork path |
| `eval_pipeline.c:593-636` | `pipeline.fork` | stage index, `npipes`, pipe fds, `job_fork()` result | each stage; replaces the bare `"forked"` at `:643` |
| `eval_node_bgnd.c:29` | `eval.background` | node, pid | `cmd &` |
| `expand_command.c:46-99` | `subst.enter/leave` | `fdstack` push, captured length, exit status | `$(…)` capture |
| `expand_arith_expr.c:128` (exists) | `expand.arith` | expression, result | `$(( ))` |

### 4.2 Redirections (`src/redir/`)

| where | event | payload |
|---|---|---|
| `redir_eval.c:21` | `redir.eval` | operator (`<`, `>`, `>>`, `<&`, `>|`, `<>`), target fd, `rfl` (`R_NOW`), word before/after expansion |
| `redir_preopen.c:25`, `redir_open.c:17` | `redir.open` | path, flags (`O_*` names), `preopen`, resulting real fd or `errno` |
| `redir_dup.c:15` | `redir.dup` | `N>&M`, `persistent`, resulting vfd link |
| `redir_here.c:7`, `redir_addhere.c` | `redir.here` | delimiter, quoted?, byte length of body |
| `redir_source.c` | `redir.source` | which source fd is redirected |

The point: an fd bug shows up as "this redirection was evaluated with these arguments and
produced that fd", **before** the deferred `fdtable_resolve()` output that is all that exists
today.

### 4.3 Builtin dispatch and fork/exec (`src/exec/`, `src/builtin/`)

| where | event | payload | answers |
|---|---|---|---|
| `exec_search.c:71`, `exec_lookup.c:7` | `exec.lookup` | name, kind found (`H_FUNCTION`, `H_SPECIAL`, `H_BUILTIN`, `H_PROGRAM`, not found), hash hit/miss, resolved `path` | why did `foo` run *that*? |
| `exec_command.c:16` | `exec.command` | `kind`, `path`, `argv`, `flag` (`X_EXEC`, `X_NOWAIT`) | the single line that says how a command will be run |
| `exec_command.c:29-44` | `exec.command.fork_builtin` | builtin forked for `&` | backgrounded builtin |
| `exec_command.c:57-79` (`H_BUILTIN`/`H_EXEC`) | `builtin.enter` / `builtin.leave` | name, `argc`, `argv`, `shell_optind` reset, pending fd opens (`fdtable_open` result), return status | builtin executed in-process; `=> status` on the way out |
| `exec_command.c:126-188` (`H_FUNCTION`) | `function.call` / `function.return` | name, positional params, `vartab_push` scope, status | function call frames |
| `exec_command.c:193` → `exec_program.c:37` | `exec.program` | path, argv, `flag`, fork needed? (`!(X_EXEC) \|\| sh->parent`) | |
| `exec_program.c:62-64` | `exec.program.pipes` | `npipes`, which fds are subst/here | `$(…)` capture wiring (exists as `fdstack_pipe`) |
| `exec_program.c:82` | `exec.program.fork` | pid or `errno` | |
| `exec_program.c:130` (parent), child after `sh_forked()` | `exec.program.pgrp` | `setpgid(pid, pid)`, `tcsetpgrp` decision | job-control correctness |
| `exec_program.c:267` (`fdtable_exec`/`fdstack_flatten`) | `fdtable.exec` | **real fd map handed to the program**, before/after | *the* fd-debugging line |
| `exec_program.c:279` | `exec.program.execve` | path, argv, count of exported vars | last thing before the image is replaced |
| after `execve()` fails (`:281`) | `exec.program.error` | `errno`, mapped status (126/127) | |
| `exec_program.c` parent, after wait | `exec.program.status` | raw wait status, mapped exit code, signal | |

Also: `builtin_timeout.c:207`, `builtin_xargs.c` (`fork()` sites) get the same `*.fork`,
`fdtable.exec`, `*.execve` events so builtins that spawn children are visible.

### 4.4 File descriptors (`src/fd*/`)

The existing prints only report *completion* of a few operations. Add the *entry* events
with parameters (call signature style), leave the existing lines as the `=>` results.

| where | event | payload |
|---|---|---|
| `fd_push.c:8`, `fd_pop.c:10` | `fd.push(n, mode)` / `fd.pop` | vfd, mode flags, `name`, stack level |
| `fd_dup.c:11` | `fd.dup(n)` | source vfd → new vfd |
| `fd_open.c:15` | `fd.open(file, mode)` | path, flags, result |
| `fd_here.c:22`, `fd_subst.c:6`, `fd_string.c`, `fd_null.c`, `fd_tempfile.c` | `fd.here` / `fd.subst` / `fd.string` / `fd.null` / `fd.tempfile` | kind of stralloc-backed fd created |
| `fd_setfd.c:51` (exists) | `fd.setfd` | drop the `-x` requirement |
| `fd_state_save.c`, `fd_state_restore.c` | `fd.state.save/restore` | `fd_expected`, `fd_hi`, `fd_lo` (the process-global bookkeeping) |
| `fdstack_push.c:7`, `fdstack_pop.c:7`, `fdstack_fork.c:6` | `fdstack.push/pop/fork` | level, number of fds released, unref'd duplicates |
| `fdstack_npipes.c`, `fdstack_pipe.c`, `fdstack_data.c` | `fdstack.npipes/pipe/data` | count, which level matched, bytes drained per subst fd (would have caught the `$(a; b)` bug immediately) |
| `fdstack_flatten.c:8`, `fdstack_update.c`, `fdstack_unref.c`, `fdstack_link.c:10` (exists) | `fdstack.flatten/update/unref/link` | fds popped, vfd remapped |
| `fdtable_lazy.c:13`, `fdtable_wish.c:7`, `fdtable_gap.c:23` | `fdtable.lazy/wish/gap` | requested real fd, force flag, resulting slot |
| `fdtable_open.c:27`, `fdtable_openfd.c`, `fdtable_close.c:13`, `fdtable_dup.c:86` (exists), `fdtable_resolve.c:113` (exists) | `fdtable.open/openfd/close/dup/resolve` | fd, flags (`MOVE`,`FORCE`,`LAZY`…), state (`DONE`/`PENDING`/`ERROR`) |
| `fdtable_exec.c:22` | `fdtable.exec` | table **before** and real fd map **after** – checkpoint #1 for "why did the child get the wrong stdout" |
| `fdtable_track.c`, `fdtable_untrack.c`, `fdtable_unexpected.c`, `fdtable_up.c`, `fdtable_link.c`, `fdtable_unlink.c` | `fdtable.track/untrack/unexpected/up/link/unlink` | placeholder fds created for gaps |
| `fd_close.c:9` | `fd.close` | vfd, real fd or "neutered (shadowed)", whether `close()` was actually called (the `fd_list[e] != fd` branches) |

### 4.5 Variables and shell environment (`src/var*/`, `src/sh/`)

| where | event | payload |
|---|---|---|
| `var_set.c:10`, `var_setv.c:10`, `var_setsa.c`, `var_setvsa.c`, `var_setvint.c` | `var.set` | name, value (truncated), flags (`V_EXPORT`, `V_READONLY`, `V_LOCAL`, …), scope depth, created vs overwritten |
| `var_unset.c:8` | `var.unset` | name, scope |
| `var_chflg.c` | `var.chflg` | name, flags before → after (`export`, `readonly`) |
| `var_import.c:12`, `var_export.c:8` | `var.import` / `var.export` | environment strings imported at start-up / exported to a child (count, plus names only) |
| `var_create.c`, `var_search.c` | `var.create` | name, which scope it landed in |
| `vartab_push.c:8`, `vartab_pop.c:7` | `vartab.push/pop` | `function` flag, scope depth, number of vars dropped |
| `sh_push.c:9`, `sh_pop.c:17` | `sh.push/pop` | new `struct env`: cwd, `$0`, positional count, `opts` bits |
| `sh_pushargs.c`, `sh_setargs.c`, `sh_popargs.c` | `sh.args` | `$#` and the positional parameters (`set --`, function call) |
| `set_apply` (`sh_main.c` option loop) | `sh.opt` | option letter, on/off (`set -e`, `set -x`, …) |
| `sh_getcwd.c`, `builtin_cd` | `sh.cwd` | old → new |
| `sh_forked.c:16` | `sh.forked` | old pid → new pid, environments discarded |
| `sh_exit.c:13` | `sh.exit` | exit code, pending traps, flushed jobs |
| `exec_search.c:15,36` (`exec_functions_save/restore`) | `func.snapshot/restore` | number of functions saved (`$(…)` scope) |
| `builtin_trap.c:411,433` (`trap_snapshot_*`) | `trap.snapshot/restore` | handle, trap count |

### 4.6 Signals and jobs (`src/job/`, `src/sh/`, `lib/sig/`, `builtin_trap.c`)

| where | event | payload |
|---|---|---|
| `sh_main.c:61` (`sh_onsig`) | `sig.handler` | signum/name, whether it reaped children (must stay async-signal-safe: buffer into a ring, flush from `trap_run_pending`) |
| `sh_main.c:423-433`, `lib/sig/sig_action.c`, `sig_catch.c` | `sig.action` | signum, handler (`SIG_DFL`/`SIG_IGN`/fn), `sa_flags` (`SA_NOCLDSTOP`, `SA_RESTART`) |
| `lib/sig/sig_block.c`, `sig_unblock.c`, `sig_blocknone.c` | `sig.block/unblock/blocknone` | signal set before/after |
| `lib/sig/sig_snapshot.c`, `sig_push.c` | `sig.snapshot/push` | dispositions captured for `trap` |
| `builtin_trap.c:99,179,196,275,340` | `trap.handler/relay/pending/uninstall/install` | signum, code, pending mask; replace the three existing `debug_to(buffer_2)` blocks |
| `job_new.c:11`, `job_fork.c:21,52,60`, `job_wait.c:24`, `job_update.c:5`, `job_signal.c:6`, `job_clean.c`, `job_foreground.c` | `job.new/fork/wait/update/signal/clean/foreground` | job id, pgrp, pids, raw `waitpid` status → `WIFEXITED/WIFSIGNALED/WIFSTOPPED`, terminal handoff (`tcsetpgrp`) |
| `job_dump.c` (exists) | `job.table` | called from `job.update` when the table changes |

### 4.7 Parsing / input (keep, but make consistent)

Port the existing `parse_*` prints to the same grammar (`parse.getarg`, `parse.command`, …)
and add `source.open(file, line)` / `source.eof` in `sh_source.c` / `sh_loop.c` so a trace
shows *which input* produced each evaluation; `sh_loop.c:53` becomes `sh.loop.list(loc=…)`.

---

## 5. Rollout order

Status: steps 1 and 2 are implemented (see [5.1](#51-implemented)); the rest is open.

1. **Foundation** – fix P1 (include `uint64.h` in `lib/buffer.h`), add `src/trace.h` +
   `src/trace/*.c` (formatter, module selector, atomic writer, `trace_reopen`), port the
   existing sites in part 1 to it and delete the old `DEBUG_<M>` flags (P5, P6, P7, P8).
2. **Exec path** (highest value): §4.3 and `fdtable.exec` in §4.4. With these, "the child got
   the wrong fds / wrong argv / wrong env" is answerable from the log alone.
3. **Evaluator + redirections**: §4.1, §4.2.
4. **fd\* internals**: the rest of §4.4.
5. **Environment, signals, jobs**: §4.5, §4.6.
6. **Tooling**: `tools/trace2seq` – turn a trace into a per-pid indented call tree or a
   sequence diagram (the grammar in §3.1 is what makes this a 50-line script); use it as an
   oracle in `tests/` (assert "this script forks exactly N times and `execve`s `/bin/cat`
   with fd 0 = `in`").

Each step is independently mergeable and, being compiled out without `DEBUG_OUTPUT`, has no
effect on the release binary.

### 5.1 Implemented

**Foundation** (`src/trace.h`, `src/trace/trace_begin.c`, `trace_value.c`, `trace_fdmap.c`)

- `TRACE()` / `TRACE_RET()` / `TRACE_STRUCT()` as in §3.2; value writers `trace_str/int/hex/raw/argv/flags`;
  every event is one `write()`; `errno` is preserved across an event; lines longer than 8 KiB are
  cut with a trailing `~`.
- Runtime selection: `SHISH_TRACE=exec,builtin,fd,...`, `all`, `-name`; `SHISH_TRACE_FILE`. Read
  once, at the first event, from the process environment (so `export SHISH_TRACE=...` inside a
  running shell is not seen; set it when starting shish).
- Modules: `exec builtin fd fdstack fdtable eval redir var sh job sig` (`enum trace_module`).
- Compiled out entirely without `DEBUG_OUTPUT`.
- P1 fixed (`lib/buffer.h` includes `uint64.h`): `DEBUG_OUTPUT` builds without `-include`.
- Ported the single-line legacy prints; they no longer need a `DEBUG_<MODULE>` flag and are
  selected at runtime instead:

| old | new |
|---|---|
| `fd_pipe n= e= ret=` | `fd.pipe(n, e, other)` |
| `fd_setfd #n e= mode=` (needed `-x`) | `fd.setfd(n, e, mode)` |
| `fd_pop 0xADDR` | `fd.pop(fd, n)` |
| `fd_close #N` | `fd.close(fd, side)` |
| `fdstack_link n=` | `fdstack.link(n)` |
| `fdstack_pipe n= fds=` | `fdstack.pipe(n, fds)` |
| `fdtable_dup #a = b` | `fdtable.dup(from, to)` |
| `"forked": pid` | `eval.pipeline.fork(pid)` |
| trap handler / uninstall / builtin_trap (stderr) | `sig.trap.handler(sig)`, `sig.trap.uninstall(sig)`, `builtin.trap(sig, code)` |

  Not ported yet (multi-line, keep using `debug.log` and their `DEBUG_*` flags): `fdtable_resolve`,
  `fdstack_data`, the `fdtable_dump` before a fork, all `parse_*`, `sh_loop`, `expr`, `expand_arith_expr`.

**Exec path** (`exec_hash.c`, `exec_command.c`, `exec_program.c`, `builtin_xargs.c`, `builtin_timeout.c`)

| event | payload |
|---|---|
| `exec.lookup` | `name, mask, cache=(hit\|miss\|path\|search), kind, path, errno` |
| `exec.command` | `kind, name, path, argc, argv, flag` |
| `exec.command.background` | `name, pid` (backgrounded builtin/function) |
| `builtin.run` / `builtin.status` | `name, argc, argv, redir_failed` / `name, status` |
| `exec.function.call` / `.return` | `name, argc, argv` / `name, status` |
| `exec.program` | `path, argv, flag, fork` |
| `exec.program.pipes` | `npipes` |
| `exec.program.fork` / `.fork_failed` | `path, pid, monitor, bgnd` / `path, errno` |
| `exec.program.child` | first line of the forked child |
| `fdtable.exec.fds { }` | **the real fds the program will inherit**, `N="/proc/self/fd/N target"` |
| `exec.program.execve` / `.execve_failed` | `path, argv, nenv` / `path, errno` |
| `exec.program.status` | `path, pid, wait (raw), exit` |
| `exec.xargs.fork` / `.execvp`, `exec.timeout.execve` | same idea for the two builtins that fork themselves |

Example (`SHISH_TRACE=exec,builtin,fdtable shish -c 'echo hi | /bin/cat'`):

```text
[3673068:1] exec.lookup(name="/bin/cat", mask=0, cache=path, kind=H_PROGRAM, path="/bin/cat", errno=0)
[3673068:1] exec.command(kind=H_PROGRAM, name="/bin/cat", path="/bin/cat", argc=1, argv=["/bin/cat"], flag=0)
[3673068:1] exec.program(path="/bin/cat", argv=["/bin/cat"], flag=0, fork=yes)
[3673068:1] exec.program.pipes(npipes=0)
[3673069:1] exec.command(kind=H_BUILTIN, name="echo", path=NULL, argc=2, argv=["echo", "hi"], flag=X_EXEC)
[3673069:1] builtin.run(name="echo", argc=2, argv=["echo", "hi"], redir_failed=0)
[3673069:1] builtin.status(name="echo", status=0)
[3673068:1] exec.program.fork(path="/bin/cat", pid=3673070, monitor=0, bgnd=0)
[3673070:1] exec.program.child(path="/bin/cat")
[3673070:1] fdtable.exec.fds { 0="pipe:[37884029]", 1="/dev/null", 2="/dev/null", 3=".../debug.log", 128="pipe:[37884028]", 129="pipe:[37884028]" }
[3673070:1] exec.program.execve(path="/bin/cat", argv=["/bin/cat"], nenv=115)
[3673068:1] exec.program.status(path="/bin/cat", pid=3673070, wait=0x0, exit=0)
```

The `fdtable.exec.fds` line already shows two things that were invisible before: the legacy
`debug.log` (fd 3) and the shell's internal pipe (128/129) leak into every exec'd program.
