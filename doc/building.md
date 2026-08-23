# Building

shish builds with CMake (the primary path) or autotools. Both produce the
same two programs:

- `shish` — the shell
- `shformat` — a pretty-printer that reuses the shell's own parser

## CMake

`cfg.sh` sources a set of helper functions that configure into
`build/<host-triple>/`, one directory per toolchain.

```sh
. ./cfg.sh                          # bring the cfg-* functions into the shell
cfg                                 # native build
cmake --build build/x86_64-linux-gnu -j
```

`cfg` passes any extra arguments straight through to `cmake`, so

```sh
cfg -DCMAKE_BUILD_TYPE=Release -DLINK_STATIC=ON -DENABLE_LTO=ON
```

configures a static, link-time-optimised build in the same layout.

### Cross-compiling

Each helper writes to its own build directory and pins its own toolchain
file where one is needed:

| helper | target |
|---|---|
| `cfg-musl`, `cfg-musl32`, `cfg-musl64` | musl libc |
| `cfg-diet`, `cfg-diet32`, `cfg-diet64` | dietlibc |
| `cfg-mingw32`, `cfg-mingw64` | Windows |
| `cfg-msys` | MSYS2 |
| `cfg-aarch64` | ARM64 Linux |
| `cfg-android`, `cfg-termux` | Android / Termux |
| `cfg-emscripten` | WebAssembly + JS glue |
| `cfg-wasm` | freestanding `wasm32` |
| `cfg-tcc` | tcc |

See [WebAssembly](wasm.md) for what to do with the last two.

### Windows (mingw)

mingw's `<signal.h>` defines no `sigaction`/mask API at all, so real
signal disposition changes (`trap CMD INT`, and anything else that
goes through `sig_action()`) always fail there -- honestly, not
silently: `sig_action()`/`sig_push()`/`sig_catch()` all return failure
rather than pretending to succeed. Signal *name*/*number* lookup
(`sig_name`, `kill -l`) is unaffected and works normally.

The `kill` builtin can still actually terminate a process: `kill`/
`kill -9`/`kill -TERM` go through `TerminateProcess`. Any other
signal name/number fails honestly instead of pretending to deliver
it -- there's no Windows equivalent for `SIGSTOP`/`SIGCONT`/etc.
`kill %job` only ever reaches a job's leading process, not its whole
process group (no real process groups exist on this platform yet).

Interactive job control (background/foreground job tracking, `Ctrl-Z`
suspend/resume, `fg`/`bg`) is off on this platform: Windows' console
model has no controlling-terminal or foreground-process-group concept
to build it on. Every job runs synchronously in the foreground instead
-- `set -m` is a no-op there.

### Options

| option | default | effect |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `MinSizeRel` | any `*Deb*` value also sets `BUILD_DEBUG=ON` |
| `LINK_STATIC` | `OFF` | static-link both programs |
| `ENABLE_LTO` | `OFF` | link-time optimisation |
| `WARN_WERROR` | `OFF` | warnings are errors |
| `USE_EFENCE` | `OFF` | link Electric Fence (debug builds) |
| `BUILD_SHFORMAT` | `ON` | build the pretty-printer |
| `BUILD_SHPARSE2AST` | `OFF` | build the AST dumper |
| `DO_CONFORMANCE_TESTS` | `ON` | register `tests/posix/*.tst` with CTest |
| `DO_YASH_TESTS` | `OFF` | register `tests/yash/*.tst` too |
| `DEBUG_OUTPUT`, `DEBUG_COLOR` | `OFF` | verbose instrumentation |
| `DEBUG_PARSE`, `DEBUG_JOB`, `DEBUG_BUILTIN`, `DEBUG_FD`, `DEBUG_FDTABLE`, `DEBUG_FDSTACK`, `DEBUG_ALLOC` | `OFF` | per-subsystem tracing |

Builtins are selected at configure time — see [Builtins](builtins.md).

## Autotools

```sh
./autogen.sh        # only from a VCS checkout
./configure
make
make install
```

`configure` looks for dietlibc under `/opt/diet` and `/usr/diet` and uses
it when it finds it. `configure --enable-builtins="..."` selects the
builtin set; `configure.ac` holds the master list.

## Tests

```sh
cd build/x86_64-linux-gnu
ctest                       # everything
ctest -R if.sh -V           # one file, verbose
```

CTest runs every `tests/*.sh` through the freshly built shell, plus the
POSIX conformance suite. [Conformance](conformance.md) explains the two
`.tst` suites and how to run a single file without a rebuild.
