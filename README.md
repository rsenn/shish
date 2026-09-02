# shish

A small POSIX-ish shell in C. One static binary, no `stdio`, no `printf` —
around 139 KB stripped, and it compiles to 184 KB of WebAssembly that runs
in a browser tab.

```sh
shish -c 'for f in *.c; do echo "${f%.c}"; done'
```

- **Small.** 139 KB stripped on x86-64 (dash 126 KB, bash 1.4 MB,
  busybox 2.0 MB), built against an in-tree copy of
  Felix von Leitner's libowfat (`lib/`) instead of the C library's
  formatted I/O.
- **Self-contained.** `cat`, `rm`, `mkdir`, `mktemp`, `ln`, `chmod`,
  `basename`, `dirname`, `uname`, `which` and friends are compile-time
  builtins. A script can run with `PATH=` empty and no `/bin` at all.
- **Portable.** CMake cross-build presets for glibc, musl, dietlibc,
  mingw32/64, MSYS, Android, Termux, aarch64, Emscripten and WASI.
- **Auditable.** ~26 k lines of C, one function per file, no dynamic
  language runtime, no history file, no startup file it did not ask for.

## Status

shish is **alpha, and not a drop-in `/bin/sh`**. It runs the shell
language — pipelines, redirections, functions, `case`, loops, parameter
expansion, arithmetic, job control, traps — and it passes 5541 cases of
yash's POSIX conformance suite, but there are 551 it still fails and a
list of known defects in [`BUGS`](BUGS). Do not put it under `init` yet.

See [Conformance](doc/conformance.md) for what actually works today, and
[`TODO.md`](TODO.md) for what is being fixed next.

## Build

```sh
cmake -S . -B build/x86_64-linux-gnu
cmake --build build/x86_64-linux-gnu -j
```

That produces `shish` (the shell) and `shformat` (a pretty-printer that
reuses the parser). See [Building](doc/building.md) for cross-compiling,
static linking, and picking which builtins get compiled in.

## Documentation

| | |
|---|---|
| [Building](doc/building.md) | toolchains, options, cross-compilation |
| [Builtins](doc/builtins.md) | what is built in, and how to choose |
| [Containers](doc/containers.md) | a shell layer that is one file |
| [Agent sandboxes](doc/agents.md) | shish as an AI harness's `/bin/sh` |
| [WebAssembly](doc/wasm.md) | emscripten and WASI builds |
| [Conformance](doc/conformance.md) | test suites and current scores |

## Name

`sh-ish`, because it is derived from `sh`; `shisha`, the water pipe, since
shells and pipes are related topics; and shish kebab, which is a nice
Turkish meal.

## Licence

GPL v2 — see [`COPYING`](COPYING). `lib/` is a copy of Felix von
Leitner's libowfat, under its own terms.
