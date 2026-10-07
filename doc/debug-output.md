# Tracing the shell: `SHISH_TRACE`

shish can write a trace of what its evaluator decides: which command was looked up, how
a word expanded, which descriptors a child inherits, when a job forked or a trap ran. One
event is one line. You choose the subsystems to follow with an environment variable and
filter the file with `grep`.

The trace is built into debug builds only. Without `DEBUG_OUTPUT` every trace point is
compiled away, so a release binary is unchanged and slower by nothing.

- [Building a shell with tracing](#building-a-shell-with-tracing)
- [Choosing what is traced](#choosing-what-is-traced)
- [Reading a trace](#reading-a-trace)
- [Modules](#modules)
- [Finding a bug with it](#finding-a-bug-with-it)
- [Tools](#tools)
- [Adding a trace point](#adding-a-trace-point)
- [Event index](#event-index)

## Building a shell with tracing

```sh
cmake -S . -B build/dbg -DCMAKE_BUILD_TYPE=Debug -DDEBUG_OUTPUT=ON     # or RelWithDebInfo
cmake --build build/dbg -j
```

`DEBUG_OUTPUT` is only declared when the build type contains `Deb` (`Debug`,
`RelWithDebInfo`) or `-DBUILD_DEBUG=ON` is given. With the default `MinSizeRel` it is
silently ignored and no trace is compiled in; an empty or missing log then means the trace
is off, not that nothing happened. Rebuild after every source change, or you trace
yesterday's code.

A debug build also defines `_DEBUG`, which force-enables the `dump` builtin.

## Choosing what is traced

Two environment variables, read once when the first event is about to be written:

| variable | meaning |
|---|---|
| `SHISH_TRACE=exec,fd` | the modules to trace. `all` selects every module, `-name` removes one (`all,-parse`). Unset or empty: no tracing. |
| `SHISH_TRACE_FILE=path` | where the lines go. Default `trace.log` in the current directory; `-` is standard error. |

```sh
rm -f trace.log
SHISH_TRACE=exec,fdtable build/dbg/shish -c 'echo hi | /bin/cat'
```

- The variable has to be in the environment when shish starts. `export SHISH_TRACE=...`
  inside a running script is not seen: put it in front of the command.
- The log is opened with `O_APPEND` and never truncated. Remove it before each run or runs
  concatenate. Child processes append to the same file.
- The file is moved to a descriptor of 200 or above and marked close-on-exec, so a script
  that redirects descriptors 1 and 2 cannot swallow it and no program inherits it. Do not
  use `SHISH_TRACE_FILE=-` when the script under test redirects standard error.
- One event is one `write(2)` of at most 8192 bytes; a longer line ends in `~`. `errno` is
  preserved across an event.

## Reading a trace

```
[2100940:1] fdtable.dup(from=4, to=1)                       a call or decision
[2100940:1] fdtable.gap => r=-3                              its result
[2100940:1] fdtable.exec.fds { 0="/dev/null", 3="pipe:[5184717]" }   a snapshot of state
```

Each line starts with `[pid:depth]`, then `module.event`, then the payload:

- `module.event(key=value, ...)` is a call or a decision with its inputs,
- `module.event => value` is a result,
- `module.event { key=value, ... }` is a snapshot of some state.

Values are integers, hexadecimal numbers, quoted strings (a missing string is `NULL`),
argument vectors `["echo", "hi"]`, flag sets `E_ROOT|E_LOOP`, source locations
`"file:line:col"`, node kinds `simple_command`, and nested `{ }` and `[ ]` groups for a
descriptor or a list of commands.

**pid** tells the processes apart. `( ... )` and `$( ... )` run in the same process (only
the depth grows); an external command, a pipeline stage and a background job each have their
own pid. So the pid column says on which side of a fork an event happened.

**depth** is the nesting of the evaluator: it grows by one inside a function call, a
subshell, a command substitution, an `eval` or a sourced file.

A sample, `SHISH_TRACE=eval,expand,exec,sh`, for `x=5 echo hi >/dev/null; f() { echo in f; }; f`
(trimmed):

```
[976472:1] eval.simple_command(loc="<string>:1:1", nassign=1, nredir=1, bgnd=0)
[976472:1] exec.lookup(name="echo", mask=0, cache=miss, kind=H_BUILTIN, path=NULL, errno=0)
[976472:1] eval.prefix_scope.enter()
[976472:1] eval.assign(var="x=5", export=1, temp=1)
[976472:1] expand.args(argv=["echo", "hi"])
[976472:1] exec.command(kind=H_BUILTIN, name="echo", path=NULL, argc=2, argv=["echo", "hi"], flag=0)
[976472:1] eval.prefix_scope.leave()
[976472:1] eval.function.define(name="f")
[976472:1] exec.function.call(name="f", argc=1, argv=["f"])
[976472:2] sh.push(cwd="/tmp/tr", argc=0, monitor=0)
[976472:2] eval.push(flags=E_FUNCTION)
[976472:2] exec.command(kind=H_BUILTIN, name="echo", path=NULL, argc=3, argv=["echo", "in", "f"], flag=0)
[976472:2] eval.pop(flags=E_FUNCTION, status=0)
[976472:1] exec.function.return(name="f", status=0)
```

## Modules

| module | what it shows | use it when |
|---|---|---|
| `parse` | tokens, commands, command lists, function definitions, arithmetic | the parser reads a script differently than you expect |
| `expand` | the argument vector after all expansions, `$(...)` | a word expands to the wrong thing |
| `eval` | node dispatch with source location, `if`/`case`/loop decisions, `&&`/`||`, assignments, `break`/`return`/`exit`, subshell and `$(...)` enter and leave, pipeline strategy | control flow or an exit status is wrong |
| `exec` | command lookup (hash hit or miss, builtin, function, program), the call itself, fork, `execve`, exit status | the wrong command ran, or ran with the wrong environment |
| `builtin` | a builtin's argument vector and status | a builtin misbehaves |
| `redir` | evaluation of each redirection, open, dup, here-document | `>`, `<&`, `exec N>` do the wrong thing |
| `fd`, `fdstack`, `fdtable` | the shell's descriptor structures: push, dup, close, pipes, the table mapping script descriptors to real ones, nesting levels | redirections, here-documents or `$(...)` output go astray |
| `var` | variable set, unset, import, export, scopes | a variable has the wrong value or leaks out of a scope |
| `sh` | start-up, environments pushed and popped, positional parameters, working directory, forks, exit | a hang, a zombie, a wrong `$@` |
| `job` | the job table, forks, waits, status decoding, terminal hand-off | job control, `wait`, `fg`/`bg` |
| `sig` | signal actions, blocking, trap installation and delivery | a trap does not run, or runs twice |

Start with the module of the symptom, not with `all`. For a descriptor problem use
`fd,fdtable`, add `fdstack,redir`, then `exec`. `all,-parse` is the widest selection that is
still readable.

### Descriptors in the exec path

Two events tell the truth about descriptors:

- `fdtable.exec.table(vfd=, shadow=, fd={n=, e=, level=, mode=})` is what the shell
  believes every script descriptor maps to just before a fork, one line per shadowed
  descriptor.
- `fdtable.exec.fds { N=target }` is what the child really has just before `execve`,
  read from `/proc/self/fd`.

A bug is where the two differ. In a sample for `echo hi | /bin/cat >/dev/null`, the child's
real descriptors are:

```
[976578:1] fdtable.exec.fds { 0="pipe:[1934834]", 1="/dev/null", 2="...", 128="pipe:[1934833]", 129="pipe:[1934833]" }
[976578:1] exec.program.execve(path="/bin/cat", argv=["/bin/cat"], nenv=91)
[976576:1] exec.program.status(path="/bin/cat", pid=976578, wait=0x0, exit=0)
```

Descriptors 128 and above are the shell's own internal pipe ends.

### Start-up events

```
sh.start(argc=5, argv=[".../shish", "-c", "echo x", "zero", "p1"], pid=49324, ppid=49272)
sh.input(kind=string, script=NULL, command="echo x", argv0="zero", args=["p1"])
sh.mode(interactive=0, monitor=0, term=0, forced=0, no_interactive=0)
```

## Finding a bug with it

1. **Reduce** the failure to a one-line `-c` script and check what `bash` or `dash` does.
2. **Trace** the module of the symptom and find the first event that is wrong. Compare the
   `fdtable.exec.table` with `fdtable.exec.fds`, or compare a passing and a failing run.
3. **Filter** before reading:

   ```sh
   grep -a '^\[2100940:' trace.log                 # one process (the child you care about)
   grep -a -E 'fdtable\.(dup|gap|wish)|fd\.setfd'   # one family of events
   grep -v exec.table trace.log | cut -c1-200       # drop snapshots, cut long lines
   sed -n '/redir.dup(/,/exec.program.execve/p' trace.log   # a window between two events
   ```

4. **Diff** a good and a bad variant after normalising what changes between runs:

   ```sh
   norm() { sed -E 's/^\[[0-9]+:/[P:/; s/0x[0-9a-f]+/0xX/g; s/pipe:\[[0-9]+\]/pipe:[N]/g'; }
   norm <good.log >good.norm
   norm <bad.log >bad.norm
   diff good.norm bad.norm
   ```

   The same script traced twice must give an empty diff; if it does not, normalise more.
   The first line that differs is usually the decision that went wrong.
5. **See what the kernel saw** with `strace`. The trace's own writes show up in it (they go to
   descriptors 200 and up), so the two logs interleave:

   ```sh
   SHISH_TRACE=fdtable SHISH_TRACE_FILE=/tmp/t.log \
     strace -f -o st.txt -s 120 -e trace=write,dup,dup2,dup3,close,fcntl,pipe2,execve shish -c '...'
   grep -a -E 'write\(2[0-9][0-9]|dup|close|execve' st.txt
   ```

   A `close(3)` right after `execve` means descriptor 3 was close-on-exec; a `dup2(a, b)`
   with no earlier relocation of `b` clobbered a live descriptor; a missing `close()` in a
   child is a leaked pipe end.
6. **Find the caller** with a debugger. The event name is the breakpoint: break in
   `trace_begin` on that event and the backtrace names the code that emitted it.

   ```sh
   SHISH_TRACE=fdtable gdb -q -batch \
     -ex 'set follow-fork-mode child' -ex 'set detach-on-fork on' \
     -ex 'break trace_begin if $_streq(event, "dup") && mod == TRACE_FDTABLE' \
     -ex run -ex 'bt 6' --args build/dbg/shish -c 'exec 3>&1; /bin/true'
   ```

   Follow the child to debug what happens after a fork, the parent to debug the code that
   forks. After the child `exec`s, gdb's "Error in re-setting breakpoint" is harmless.

Timing-sensitive races (`sig`, `wait`) change under `strace` and a debugger; tracing alone
disturbs them least, so compare a run with and without `strace`.

## Tools

**`tools/trace2seq`** turns a log into something shorter to read. It needs `exec` and `sh` in
`SHISH_TRACE`.

```sh
tools/trace2seq trace.log            # process tree: who forked whom, what each pid exec'd, how it exited
tools/trace2seq -c EVENT trace.log   # number of lines of one event, e.g. exec.program.execve
tools/trace2seq -s trace.log         # Mermaid sequence diagram of forks, execs and exits
```

```
pid 976576 shell exit=0
  pid 976577 shell exit=0
  pid 976578 exec /bin/cat ["/bin/cat"] exit=0
```

`tests/trace2seq.sh` uses it as an oracle: it asserts that a script forks and `execve`s the
expected number of programs.

**`dump`** (builtin, enabled by a debug build or `-DBUILTIN_DUMP=ON`) prints internal state to
a descriptor on request: `-v` the root variable table, `-l` the innermost local table, `-F`
the defined functions, and in a debug build `-t` the descriptor table, `-s` the descriptor
stack, `-f` the descriptor list, `-j` the jobs. `-u fd` chooses where the dump goes.

**`shparse2ast`** (`-DBUILD_SHPARSE2AST=ON`) prints the full syntax tree of a script as JSON.
The trace only names node kinds and source text; use this when the whole tree matters.

**ASan and UBSan** (`-DCMAKE_C_FLAGS="-fsanitize=address,undefined"`, run with
`ASAN_OPTIONS=detect_leaks=0` unless hunting leaks) find memory errors; `valgrind` does for a
build without a sanitizer.

## Adding a trace point

```c
TRACE(TRACE_FDTABLE, "dup", trace_int("from", o), trace_int("to", e));   /* decision and inputs */
TRACE_RET(TRACE_FDTABLE, "gap", trace_int("r", r));                      /* result */
TRACE_STRUCT(TRACE_FDTABLE, "state", trace_fd("fd", d));                 /* snapshot */
```

- Name the event `module.event`. Put the inputs of a decision in the call and the outcome in
  a `TRACE_RET`; place the `TRACE_RET` after the operation, or the line appears before it
  finished. Prefer one event per branch that can go wrong over a dump of everything.
- The arguments are evaluated only when the module is selected, and they disappear without
  `DEBUG_OUTPUT`. Never put a side effect in them. A variable used only inside a trace point
  may need a `(void)` cast to stay warning-free in a release build.
- Value writers: `trace_int`, `trace_hex`, `trace_str` (a null string prints `NULL`),
  `trace_strn`, `trace_raw`, `trace_argv`, `trace_flags(key, bits, names, n)`, `trace_loc`,
  `trace_kind`, `trace_node`, `trace_nodes`, `trace_fd`, and `trace_open` / `trace_close` for a
  nested group. `trace_fdtable(event)` and `trace_fdmap(event)` dump a whole table. A new type
  gets its writer in `src/trace/trace_value.c`.
- **Never call `TRACE` from a signal handler.** Use `TRACE_DEFER(mod, ev, key, val)` there and
  `trace_flush()` from normal context.
- A new module is an entry in `enum trace_module` (`src/trace.h`) and its name in
  `trace_names[]` in `src/trace/trace_begin.c`, in the same order.
- Leave useful trace points in. Do not leave `fprintf`, `write(2, ...)` or `abort()` debugging
  in a commit, and keep `trace.log`, `strace` output and core files out of `git add`.

## Event index

Every trace point in `src/`, by module, with the keys of its payload. A `TRACE_RET` event
is printed as `=> value` and a `TRACE_STRUCT` event as `{ ... }`, as described above.
Add a row when you add a trace point
(`grep -rn 'TRACE.*(TRACE_<MODULE>, "event"' src` shows its keys).

**`exec`**

| event | payload | file |
|---|---|---|
| `exec.command` | argc, argv, flag, kind, name, path | `exec_command.c` |
| `exec.command.background` | name, pid | `exec_command.c` |
| `exec.func.restore` | count, depth | `exec_search.c` |
| `exec.func.snapshot` | count, depth | `exec_search.c` |
| `exec.function.call` | argc, argv, name | `exec_command.c` |
| `exec.function.return` | name, status | `exec_command.c` |
| `exec.lookup` | cache, errno, kind, mask, name, path | `exec_hash.c` |
| `exec.program` | argv, flag, fork, path | `exec_program.c` |
| `exec.program.child` | path | `exec_program.c` |
| `exec.program.execve` | argv, nenv, path | `exec_program.c` |
| `exec.program.execve_failed` | errno, path | `exec_program.c` |
| `exec.program.fork` | bgnd, monitor, path, pid | `exec_program.c` |
| `exec.program.fork_failed` | errno, path | `exec_program.c` |
| `exec.program.pipes` | npipes | `exec_program.c` |
| `exec.program.status` | exit, path, pid, wait | `exec_program.c` |

**`builtin`**

| event | payload | file |
|---|---|---|
| `builtin.run` | argc, argv, name, redir_failed | `exec_command.c` |
| `builtin.status` | name, status | `exec_command.c` |
| `builtin.trap` | code, sig | `builtin_trap.c` |

**`fd`**

| event | payload | file |
|---|---|---|
| `fd.close` | fd, side | `fd_close.c` |
| `fd.dup` | n, name, of, src | `fd_dup.c` |
| `fd.here` | len, n | `fd_here.c` |
| `fd.null` | n | `fd_null.c` |
| `fd.open` | file, mode, n | `fd_open.c` |
| `fd.pipe` | e, n, other | `fd_pipe.c` |
| `fd.pop` | fd, n | `fd_pop.c` |
| `fd.push` | level, mode, n | `fd_push.c` |
| `fd.setfd` | e, mode, n | `fd_setfd.c` |
| `fd.state.restore` | expected, hi, lo | `fd_state_restore.c` |
| `fd.state.save` | expected, hi, lo | `fd_state_save.c` |
| `fd.string` | len, n | `fd_string.c` |
| `fd.subst` | n | `fd_subst.c` |
| `fd.tempfile` | e, file, n | `fd_tempfile.c` |

**`fdstack`**

| event | payload | file |
|---|---|---|
| `fdstack.data` | bytes, fd | `fdstack_data.c` |
| `fdstack.flatten` | level | `fdstack_flatten.c` |
| `fdstack.fork` | n | `fdstack_fork.c` |
| `fdstack.link` | n | `fdstack_link.c` |
| `fdstack.npipes` | mode, n | `fdstack_npipes.c` |
| `fdstack.pipe` | fds, n | `fdstack_pipe.c` |
| `fdstack.pop` | level | `fdstack_pop.c` |
| `fdstack.push` | level | `fdstack_push.c` |
| `fdstack.unref` | dupes, n | `fdstack_unref.c` |
| `fdstack.update` | e, n | `fdstack_update.c` |

**`fdtable`**

| event | payload | file |
|---|---|---|
| `fdtable.close` | e, flags, r | `fdtable_close.c` |
| `fdtable.dup` | from, to | `fdtable_dup.c` |
| `fdtable.gap` | e, flags, r | `fdtable_gap.c` |
| `fdtable.lazy` | e, expected, flags, r | `fdtable_lazy.c` |
| `fdtable.link` | n | `fdtable_link.c` |
| `fdtable.open` | file, flags, n, r | `fdtable_open.c` |
| `fdtable.openfd` | e, flags, n, r | `fdtable_openfd.c` |
| `fdtable.resolve` | fd, flags, state | `fdtable_resolve.c` |
| `fdtable.track` | expected, flags, n | `fdtable_track.c` |
| `fdtable.unexpected` | e, flags, u | `fdtable_unexpected.c` |
| `fdtable.unlink` | n | `fdtable_unlink.c` |
| `fdtable.untrack` | e, expected | `fdtable_untrack.c` |
| `fdtable.up` | expected | `fdtable_up.c` |
| `fdtable.wish` | e, expected, flags, r | `fdtable_wish.c` |

**`eval`**

| event | payload | file |
|---|---|---|
| `eval.and_or` | left, op, run_right | `eval_and_or.c` |
| `eval.and_or.status` | op, status | `eval_and_or.c` |
| `eval.assign` | export, temp, var | `eval_simple_command.c` |
| `eval.background` | kind, pid | `eval_node_bgnd.c` |
| `eval.case.match` | pattern, word | `eval_case.c` |
| `eval.case.nomatch` | word | `eval_case.c` |
| `eval.errexit` | status | `eval_cmdlist.c, eval_tree.c` |
| `eval.exit` | code, found | `eval_exit.c` |
| `eval.for.iter` | value, var | `eval_for.c` |
| `eval.function.define` | name | `eval_function.c` |
| `eval.function.redefine` | name | `eval_function.c` |
| `eval.if.branch` | taken | `eval_if.c` |
| `eval.if.test` | status | `eval_if.c` |
| `eval.jump` | cont, found, levels | `eval_jump.c` |
| `eval.loop` | kind | `eval_loop.c` |
| `eval.loop.test` | continue, status | `eval_loop.c` |
| `eval.node` | flags, kind, loc | `eval_node.c` |
| `eval.pipeline` | bgnd, filter_chain, lastpipe, stages | `eval_pipeline.c` |
| `eval.pipeline.fork` | pid | `eval_pipeline.c` |
| `eval.pop` | flags, status | `eval_pop.c` |
| `eval.prefix_scope.enter` | – | `eval_simple_command.c` |
| `eval.prefix_scope.leave` | – | `eval_simple_command.c` |
| `eval.push` | flags | `eval_push.c` |
| `eval.redir_scope.enter` | kind, nredir | `eval_command.c` |
| `eval.redir_scope.leave` | status | `eval_command.c` |
| `eval.return` | found, value | `eval_return.c` |
| `eval.simple_command` | bgnd, loc, nassign, nredir | `eval_simple_command.c` |
| `eval.simple_command.status` | status | `eval_simple_command.c` |
| `eval.status` | kind, status | `eval_cmdlist.c, eval_tree.c` |
| `eval.subshell.enter` | – | `eval_subshell.c` |
| `eval.subshell.leave` | status | `eval_subshell.c` |
| `eval.subst.enter` | – | `expand_command.c` |
| `eval.subst.leave` | len, status | `expand_command.c` |

**`expand`**

| event | payload | file |
|---|---|---|
| `expand.args` | argv | `eval_simple_command.c` |
| `expand.arith.unsupported` | kind | `expand_arith_expr.c` |

**`redir`**

| event | payload | file |
|---|---|---|
| `redir.dup` | fd, persistent, src | `redir_dup.c` |
| `redir.dup.self` | fd | `redir_eval.c` |
| `redir.eval` | fd, flag, preallocated, target | `redir_eval.c` |
| `redir.eval.status` | fd, status | `redir_eval.c` |
| `redir.here` | fd, len | `redir_here.c` |
| `redir.open` | fd, mode, now, path, preopen | `redir_open.c` |
| `redir.preopen` | errno, fd, path, result | `redir_preopen.c` |

**`var`**

| event | payload | file |
|---|---|---|
| `var.chflg` | before, flags, name, set | `var_chflg.c` |
| `var.create` | depth, name, shadows | `var_create.c` |
| `var.export` | count | `var_export.c` |
| `var.set` | flags, level, v | `var_set.c` |
| `var.setv` | flags, level, name, value | `var_setv.c` |
| `var.unset` | level, name | `var_unset.c` |
| `var.vartab.pop` | function, level | `vartab_pop.c` |
| `var.vartab.push` | function, level | `vartab_push.c` |

**`sh`**

| event | payload | file |
|---|---|---|
| `sh.args` | argc, argv | `sh_setargs.c` |
| `sh.cwd` | new, old | `builtin_cd.c` |
| `sh.exit` | child, code | `sh_exit.c` |
| `sh.fmt.config` | columnwrap, indent_width | `sh_fmt.c` |
| `sh.forked` | new, old | `sh_forked.c` |
| `sh.getcwd` | cwd | `sh_getcwd.c` |
| `sh.input` | args, argv0, command, kind, script | `sh_main.c` |
| `sh.loop.list` | cmds, n | `sh_loop.c` |
| `sh.mode` | forced, interactive, monitor, no_interactive, term | `sh_main.c` |
| `sh.opt` | letter, on | `builtin_set.c` |
| `sh.pop` | cwd, exitcode | `sh_pop.c` |
| `sh.popargs` | argc | `sh_popargs.c` |
| `sh.push` | argc, cwd, monitor | `sh_push.c` |
| `sh.pushargs` | argc | `sh_pushargs.c` |
| `sh.start` | argc, argv, pid, ppid | `sh_main.c` |

**`job`**

| event | payload | file |
|---|---|---|
| `job.clean` | id, pgrp | `job_clean.c` |
| `job.foreground` | id, pgrp | `job_foreground.c` |
| `job.fork` | bgnd, id, pgrp, pid | `exec_program.c, job_fork.c` |
| `job.fork.child` | bgnd, pgrp | `job_fork.c` |
| `job.new` | id, nproc | `job_new.c` |
| `job.reap` | pid, status | `job_wait.c` |
| `job.signal` | pid | `job_signal.c` |
| `job.table` | njobs | `job_update.c` |
| `job.update` | – | `job_update.c` |
| `job.wait` | id, pgrp, pid, status | `job_wait.c` |

**`sig`**

| event | payload | file |
|---|---|---|
| `sig.action` | flags, handler, sig | `sh_main.c` |
| `sig.block` | sig | `exec_program.c, job_foreground.c, job_fork.c, job_update.c` |
| `sig.blocknone` | – | `exec_program.c` |
| `sig.handler` | sig | `sh_main.c` |
| `sig.push` | handler, sig | `builtin_trap.c` |
| `sig.trap.deferred` | sig | `builtin_trap.c` |
| `sig.trap.handler` | sig | `builtin_trap.c` |
| `sig.trap.install` | ignore, name, sig | `builtin_trap.c` |
| `sig.trap.pending` | sig | `builtin_trap.c` |
| `sig.trap.relay` | sig | `builtin_trap.c` |
| `sig.trap.restore` | count | `builtin_trap.c` |
| `sig.trap.scope.default` | sig | `builtin_trap.c` |
| `sig.trap.snapshot` | count | `builtin_trap.c` |
| `sig.trap.uninstall` | sig | `builtin_trap.c` |
| `sig.unblock` | sig | `exec_program.c, job_foreground.c, job_fork.c, job_update.c` |

**`parse`**

| event | payload | file |
|---|---|---|
| `parse.arith` | kind | `parse_arith.c` |
| `parse.command` | kind | `parse_command.c` |
| `parse.expect_failed` | discarded | `parse_expect.c` |
| `parse.function` | name | `parse_function.c` |
| `parse.getarg` | word | `parse_getarg.c` |
| `parse.grouping` | kind | `parse_grouping.c` |
| `parse.list` | cmds, n | `parse_list.c` |
| `parse.simple_command` | loc, text | `parse_simple_command.c` |
| `parse.token` | flags, tok | `parse_gettok.c` |
