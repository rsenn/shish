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
basename cat chmod dirname hostname ln mkdir mktemp rm rmdir uname which
```

These are off by default. Turn them on and a script stops needing
coreutils:

```sh
$ shish -c 'PATH=; mkdir -p a/b; echo hi > a/b/f; cat a/b/f; rm -r a'
hi
```

## Choosing

```sh
cfg -DENABLE_ALL_BUILTINS=ON              # everything
cfg -DENABLE_CAT=ON -DENABLE_MKDIR=ON     # just these two on top of the default set
cfg -DENABLE_HISTORY=OFF                  # and this one off
```

The configure step writes `<builddir>/src/builtin_config.h`, which is
what `src/builtin/builtin_table.c` is compiled against. Nothing you left
out is linked in.

With autotools, the same choice is
`./configure --enable-builtins="cat mkdir rm"`.

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
