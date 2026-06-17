# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`shish` is a small POSIX-ish shell written in C. It targets the IEEE P1003.2
draft. It deliberately avoids `stdio` and printf-style formatting by linking
against an in-tree copy of Felix von Leitner's `libowfat` (under `lib/`) and
using as few libc facilities as possible (mostly POSIX syscall wrappers). The
codebase is "proof-of-concept" quality per the project's own README and is
**not** a drop-in replacement for `sh`/`bash`.

Two build executables are produced:
- `shish` — the shell (`src/sh/sh_main.c`)
- `shformat` — pretty-printer that reuses the parser (`src/sh/sh_fmt.c`)
- `shparse2ast` — optional AST dumper (off by default; `BUILD_SHPARSE2AST=ON`)

## Build

The repo supports both CMake and autotools, but CMake is the primary path.

### CMake (preferred)

There is no top-level out-of-tree convention; use `cfg-cmake.sh` (sourced via
`cfg.sh`) which dispatches to per-toolchain helpers and writes into
`build/<host-triple>/`.

```sh
. ./cfg.sh                 # source the cfg-* functions into the shell
cfg                        # default native build (writes build/<triple>/)
cmake --build build/x86_64-linux-gnu -j
```

Other helpers in `cfg-cmake.sh`: `cfg-diet`, `cfg-diet32`, `cfg-diet64`,
`cfg-musl`, `cfg-musl32`, `cfg-musl64`, `cfg-mingw32`, `cfg-mingw64`,
`cfg-emscripten`, `cfg-wasm`, `cfg-tcc`, `cfg-aarch64`, `cfg-android`,
`cfg-termux`, `cfg-msys`. Each cross-build writes to its own `build/<host>/`
and may pin a toolchain file.

Common CMake options (pass with `-D…` after the `cfg` function or directly
to `cmake`):
- `CMAKE_BUILD_TYPE={Debug,Release,MinSizeRel,RelWithDebInfo}` — default is
  `MinSizeRel`; any `*Deb*` flips `BUILD_DEBUG=ON` which defines `_DEBUG=1` and
  force-enables the `dump` builtin.
- `LINK_STATIC=ON` — static-link the executables.
- `ENABLE_LTO=ON`, `USE_EFENCE=ON` (debug builds), `WARN_WERROR=ON`.
- `DEBUG_OUTPUT`, `DEBUG_COLOR`, `DEBUG_ALLOC` — verbose debug instrumentation;
  `DEBUG_PARSE`, `DEBUG_JOB`, `DEBUG_BUILTIN` are per-subsystem (see
  `cmake/Debug.cmake`).
- `BUILD_SHFORMAT=ON` (default), `BUILD_SHPARSE2AST=OFF`.
- `NO_TREE_PRINT=ON` — strip tree-printing helpers from history.
- Builtins are individually toggleable. `cmake/Builtins.cmake` enumerates
  `MINIMAL_BUILTINS`, `DEFAULT_BUILTINS`, `EXTRA_BUILTINS`; pass
  `-DENABLE_ALL_BUILTINS=ON` for everything, or `-DENABLE_<NAME>=ON/OFF`
  per builtin. The generated `build/.../src/builtin_config.h` is what
  `src/builtin/builtin_table.c` is compiled against.

### Autotools (alternative)

```sh
./autogen.sh   # only needed from a VCS checkout (runs aclocal+autoheader+autoconf)
./configure    # detects dietlibc under /opt/diet or /usr/diet automatically
make
make install
```

`configure --enable-builtins="..."` selects which builtins get compiled in
(see `configure.ac` for the master list).

## Tests

Tests are shell scripts under `tests/` (top-level `*.sh`, plus `tests/posix/`
and `tests/yash/`). CMake registers each top-level `tests/*.sh` (excluding
`common.sh`) as a CTest target that runs the script through the freshly built
`shish` binary.

```sh
cd build/x86_64-linux-gnu
ctest                          # run all tests
ctest -R if.sh -V              # run one test, verbose
./shish ../../tests/if.sh      # invoke a test directly through shish
```

`tests/common.sh` defines the `assert_equal`, `assert_match`, `success`,
`failure`, `summary` helpers; each test sources it via
`. "$(dirname "$0")/common.sh"`. A test "fails" by calling `failure` which
prints `FAILURE` and `exit 1`s.

## Code architecture

The shell follows the classic source → parser → tree → evaluator pipeline.
The whole shell lives under `src/`, with one subdirectory per subsystem and
matching `<subsystem>.h` at `src/`'s root.

- `src/source/` + `src/source.h` — input layer. `struct source` wraps an `fd`
  (file, mmap, string, or stdin/terminal) and feeds the parser. `source_push`
  stacks sources (e.g. `.` / `source` builtin, here-docs).
- `src/parse/` + `src/parse.h` — recursive-descent parser. Token character
  classification is table-driven via `parse_chartable[]` (see
  `parse_chartable.c`) with `parse_is{space,name,ctrl,esc,…}` inline macros in
  `parse.h`. Each grammar production (`parse_if`, `parse_for`, `parse_case`,
  `parse_pipeline`, `parse_simple_command`, `parse_arith_*`, etc.) has its own
  file. Nodes are allocated via `parse_newnode.c` (or `parse_newnodedebug.c`
  when `DEBUG_ALLOC=ON`).
- `src/tree/` + `src/tree.h` — the AST. `enum kind` (N_SIMPLECMD, N_PIPELINE,
  N_AND/OR/NOT, N_LIST, N_SUBSHELL, N_BRACEGROUP, N_FOR, N_CASE, N_IF, N_WHILE,
  N_UNTIL, N_FUNCTION, N_ARG*, N_ASSIGN, N_REDIR, plus an `A_*` arithmetic
  sub-tree) drives a tagged `union node`. All node structs are `__packed`.
  `tree_print.c`/`tree_printlist.c` exist for debug/history dumping and can be
  stripped via `NO_TREE_PRINT`.
- `src/eval/` + `src/eval.h` — tree walker. Entry is `eval_tree` /
  `eval_node` / `eval_command`. There's one `eval_<construct>.c` per node
  kind. Control-flow constructs (`break`/`continue`/`return`/`exit`) use a
  `jmp_buf` on each `struct eval` and `eval_jump` to unwind. Loops, function
  bodies and subshells push/pop `struct eval` frames via `eval_push`/`eval_pop`.
- `src/expand/` + `src/expand.h` — word expansion (parameter, command,
  arithmetic, tilde, splitting, glob) applied to `N_ARG*` nodes before
  execution.
- `src/exec/`, `src/fork.c` — process spawn, PATH search, exec.
- `src/redir/`, `src/fd/`, `src/fdstack/`, `src/fdtable/` — redirection. The
  fdtable lazily resolves redirections; per BUGS/TODO this is the source of
  several known issues.
- `src/var/`, `src/vartab/` — variable storage with a stack of `struct vartab`
  per env frame; `var_import` populates from `envp` at startup.
- `src/builtin/` — one C file per builtin (`builtin_<name>.c`).
  `src/builtin/builtin_table.c` is compiled with
  `-DBUILTIN_<NAME>=1` flags derived from `build/.../src/builtin_config.h` so
  the dispatch table only references compiled-in builtins. `builtin_search.c`
  does the name lookup. Two builtin classes: `B_SPECIAL` (POSIX special
  builtins, e.g. `exec`, `exit`, `set`) and default.
- `src/job/` — job control (`SIGCHLD` handler in `sh_main.c`,
  `job_signal`/`wait_nohang`).
- `src/term/`, `src/prompt/`, `src/history/` — interactive terminal layer.
  `term_init` is what flips the shell into interactive mode if `fd_src` is a
  character device.
- `src/sh/` + `src/sh.h` — top-level glue. `struct env` (the per-frame state:
  `cwd`, `umask`, `shopt`, `fdstack`, `varstack`, `arg`, parser, eval, finalizers)
  is the central data structure; the active frame is `sh`. `sh_main.c` does
  argv parsing, env import, source/term setup, then `sh_loop()`.

External dependency `libowfat` lives in-tree at `lib/` and is built as a
static `libowfat.a` by `cmake/libowfat.cmake` (only if no system libowfat is
detected). `libshell.a` is the rest of the shell as a static lib, which both
`shish` and `shformat` link.

## Code style

- C99. Indentation 2 spaces, no tabs, K&R-ish braces (`BreakBeforeBraces: Attach`,
  `AlwaysBreakAfterDefinitionReturnType: TopLevel`). See `.clang-format`.
- Reformat with `cmake --build build/<dir> --target clang-format` or
  `cmake --build build/<dir> --target cmake-format`.
- Headers are guarded with `#ifndef <SUBSYS>_H` and `<subsys>.h` lives at
  `src/`'s root, while implementations live in `src/<subsys>/<subsys>_<verb>.c`.
- Avoid `stdio.h` — use `lib/buffer/`, `lib/fmt/`, `lib/scan/`, `lib/stralloc/`
  helpers from libowfat instead. The shell currently includes `<stdlib.h>` but
  not `<stdio.h>` deliberately.
- `HAVE_CONFIG_H` is always defined; gate platform-conditional code on the
  `HAVE_*` macros emitted into `config.h` (CMake) or `config.h.in` (autotools).
