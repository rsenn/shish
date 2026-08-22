# shish in agent sandboxes

An AI coding agent runs shell commands — usually a lot of them, usually
inside a sandbox someone has to reason about. The shell in that sandbox is
part of the security boundary and part of the reproducibility story, and
`bash` is a strange choice for both: 1.4 MB, a startup-file search path,
history, aliases, a job-control layer, `$BASH_ENV`, and a language that
nobody can fully enumerate.

shish is 185 KB of C that you can read.

## What matters for a harness

**A small, enumerable surface.** ~26 k lines, one function per file, no
dynamic runtime, no plugins. When a sandbox review asks "what can the
shell do", the answer fits in a `src/builtin/` listing.

**No implicit state.** shish reads no startup file of its own. `$ENV` is
the only hook, and `-p` makes the shell ignore even that:

```sh
shish -p -c "$COMMAND"      # nothing sourced, nothing inherited but the environment
```

There is no history file to write, no `~/.shishrc` to plant, and no alias
expansion unless the script itself defines one.

**Determinism across environments.** The same static binary is the shell
in the container, on the CI runner, and in the developer's checkout, so a
command that works in one works in the others. Cross-builds for musl,
dietlibc, aarch64, Android/Termux, Windows and WebAssembly all come from
one tree — see [Building](building.md).

**Cheap.** Startup is ~1.3 ms per `shish -c true` on a warm cache, against
~1.8 ms for bash. An agent that runs thousands of short commands per
session notices.

**Parse before you run.** `shish -n script` parses and reports syntax
errors without executing anything — a cheap validity check on a command an
agent just generated:

```sh
$ shish -n script.sh; echo $?
script.sh:2:1: unexpected token EOF, expecting 'fi'
1
```

(With `-c` instead of a file the message is printed but the status is
still 0 — see [`BUGS`](../BUGS): `syntax-error-in-c-string-exits-zero`.)

## Running untrusted commands

shish is a shell, not a sandbox: it executes what it is told to. Keep the
kernel-level boundary you already have — namespaces, seccomp, a read-only
root, a network policy. What shish changes is how much *else* is inside
that boundary:

```sh
cfg-musl -DLINK_STATIC=ON -DENABLE_ALL_BUILTINS=ON
```

gives you a sandbox whose entire userland can be one file, with `PATH=`
set to nothing — see [Containers](containers.md). An agent can still
`mkdir`, `cat`, `rm` and loop; it cannot reach a binary that is not there.

## In the browser

The same shell compiles to 184 KB of WebAssembly, which means an agent
UI can run its shell commands client-side, in a tab, with no server and
no container at all. See [WebAssembly](wasm.md) and the
[playground](../play.html).

## Caveat

shish is alpha. It passes 5270 cases of yash's POSIX suite and fails 822
of them ([Conformance](conformance.md)); the known defects are listed in
[`BUGS`](../BUGS). Pin a commit, run your own command corpus through it,
and report what breaks.
