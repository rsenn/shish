# Builtins

Every builtin is a compile-time choice. The shell you ship contains the
ones you asked for and nothing else — which is what makes both the
[container](containers.md) and the [WebAssembly](wasm.md) story work: the
utilities a script needs are inside the binary, not on a `PATH`.

## The three sets

`cmake/Builtins.cmake` defines them.

**Minimal** — the shell language itself, plus what POSIX requires a shell
to provide:

```
. : alias break cd command eval exec exit export expr getopts hash
history jobs kill local printf pwd read readonly return set shift
source test times trap type umask unset wait
```

**Default** — the minimal set plus `echo`, `true`, `false`, `help`,
`type` and `fdtable`. This is what a plain `cfg` build gives you.

**Extra** — the reason a shish container image can be a single file:

```
basename cat chmod digest dirname find grep hostname link ln ls mkdir
mktemp readlink realpath rm rmdir sed sleep tee timeout touch uname wc which
```

These are off by default. Turn them on and a script stops needing
coreutils (or `grep`/`sed`):

```sh
$ shish -c 'PATH=; mkdir -p a/b; echo hi > a/b/f; cat a/b/f; rm -r a'
hi
$ shish -c 'PATH=; printf "a\nb\nc\n" | grep b | sed s/b/B/'
B
```

`src/builtin/extra/` is where these live in the tree (as opposed to
`src/builtin/` for the language's own required builtins); a builtin goes
there when it stands in for an external program rather than implementing
shell syntax. `find`, `grep` and `sed` are the larger ones and get their
own subsystem under `text/` (see "Regex backend" below) instead of living
entirely in one `builtin_*.c` file.

## Choosing

```sh
cfg -DENABLE_ALL_BUILTINS=ON              # everything
cfg -DBUILTIN_CAT=ON -DBUILTIN_MKDIR=ON   # just these two on top of the default set
cfg -DBUILTIN_HISTORY=OFF                 # and this one off
```

(`-DENABLE_<NAME>=ON/OFF` also works, as the older spelling of
`-DBUILTIN_<NAME>`.)

The configure step writes `<builddir>/src/builtin_config.h`, which is
what `src/builtin/builtin_table.c` is compiled against. Nothing you left
out is linked in.

With autotools, the same choice is
`./configure --enable-builtins="cat mkdir rm"`.

## Regex backend: `text/dfa` or `regex.h`

`expr` and `sed` always match via `text/dfa` (an in-tree, from-scratch
POSIX BRE/ERE engine, `text/dfa.h`) — no libc dependency, so both work
the same way on a static/musl/dietlibc/WASM build as anywhere else. `sed`
additionally needs `s///`'s substitution machinery (`dfa_replace()`),
which has no `<regex.h>` equivalent, so it has no backend switch at all.

`grep` can go either way:

```sh
cfg -DGREP_USE_SYSTEM_REGEX=ON   # link the system's <regex.h> instead
```

Off (the default) is what makes `grep` work on a target with no libc
regex implementation at all (WASI's libc has none); on is smaller where
glibc/musl's `regex.h` is already being linked in for other reasons, and
picks up the host's own conformance/locale behavior instead of `text/dfa`'s.
Either way `grep`'s matching results should agree — see `tests/dev/dfa-diff.c`
for the differential test between the two.

## What a builtin costs

Not much. A default dynamic build is 139 KB stripped (132 KB of text);
the same build with `-DENABLE_ALL_BUILTINS=ON` is 164 KB (154 KB of
text) — 25 KB for the whole extra set. Static against glibc, with
everything on, it is 1.05 MB.

## Precedence

A builtin is found before `PATH` is searched, as POSIX requires for the
special builtins and permits for the rest. To reach the real binary
anyway, call it by path:

```sh
/bin/cat file        # the coreutils one
command -p cat file  # the standard utility, via the default PATH
```

`type name` says which one you would get.
