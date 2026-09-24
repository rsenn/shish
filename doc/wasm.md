# shish on WebAssembly

shish compiles to a 184 KB `.wasm` module. That is a whole POSIX-ish
shell — pipelines, functions, parameter expansion, arithmetic,
redirections — running in a browser tab, a serverless worker, or any
WASI runtime, with no process and no container underneath it.

Try it in the [playground](../play.html).

## Emscripten (browser, with JS glue)

```sh
CC=emcc CXX=em++ cmake -S . -B build/emscripten \
  -DCMAKE_TOOLCHAIN_FILE="$(dirname "$(which emcc)")/cmake/Modules/Platform/Emscripten.cmake" \
  -DCMAKE_EXE_LINKER_FLAGS="-s WASM=1 -sEXPORTED_RUNTIME_METHODS=['callMain'] -sINVOKE_RUN=0" \
  -DCMAKE_EXECUTABLE_SUFFIX=".html" -DENABLE_SHARED=OFF -DENABLE_PIC=FALSE
cmake --build build/emscripten -j
```

That produces `build/emscripten/shish.js` and `shish.wasm`. Add
`-DENABLE_ALL_BUILTINS=ON` (see [Builtins](builtins.md)) to also get
`cat`/`mkdir`/`grep`/`sed`/etc. as in-process builtins instead of just the
minimal/default set — this is what the [playground](../play.html) itself
is built with (`build/emscripten-all/`, 253 KB vs. 184 KB minimal); the
[site's `files` config](https://github.com/rsenn/rsenn/blob/main/sites/shish/site.config.js)
copies straight from that directory, so rebuild it there before publishing.
The linker flags above already pass what the glue needs:

```
-sEXPORTED_RUNTIME_METHODS=['callMain'] -sINVOKE_RUN=0
```

so the module loads without running anything, and each script is one
`callMain()`:

```html
<script>
  var Module = {
    print: function (line) { output(line); },
    printErr: function (line) { output(line); },
    onRuntimeInitialized: function () { ready(); },
  };
</script>
<script src="shish.js"></script>
<script>
  function run(script) {
    try {
      Module.callMain(['-c', script]);
    } catch (e) {
      // exit() unwinds the wasm stack by throwing; that is not an error
      if (!(e && e.name === 'ExitStatus')) throw e;
    }
  }
</script>
```

`web/index.html` in the repo is exactly this, in 60 lines.

### What works, and what does not

Everything that does not need a process does. Expansion, arithmetic,
control flow, functions, here-documents, variables, the file-utility
builtins against Emscripten's in-memory filesystem.

What cannot work in the browser is anything that forks: pipelines between
two external commands, background jobs, `$(...)` that runs a program.
There are no processes to fork. Compile the utilities you need in as
[builtins](builtins.md) and they run in-process instead.

## WASI (wasmtime, wasmer, Node, webassembly.sh)

Needs [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) (tested with 34,
`/opt/wasi-sdk`; set `WASI_SDK_PREFIX` otherwise):

```sh
. ./cfg-cmake.sh
cfg-wasi                       # writes build/wasi/, MinSizeRel, all builtins
cmake --build build/wasi -j
```

This produces `build/wasi/shish`, a 248 KB `wasm32-wasip1` module. wasi-libc
has no `fork`, `exec`, `wait`, `termios`, `pwd` or `sigaction`, so
`src/wasi/wasi_compat.h` (force-included on this target only) supplies
stubs that fail the way a system without the feature does. The build also
links wasi-libc's `-lwasi-emulated-{signal,getpid,process-clocks,mman}`.

### Runtime requirement: WebAssembly exceptions

`setjmp`/`longjmp` carry subshells, pipelines, functions, `break` and
`return`, and wasi-sdk implements them with WebAssembly exception handling
(the `exnref` form; the older opcodes are no longer emitted). The engine
must support it:

```sh
node --experimental-wasm-exnref run.mjs build/wasi/shish -c 'echo hello'   # Node 23
```

Node 24+, recent wasmtime and browsers that ship `exnref` need no flag.
Whether a given host supports it has to be checked per host: if it does
not, instantiation fails before any shell code runs.

### webassembly.sh

Tested 2026-09-19 with Chrome 152: run `wapm upload` in the terminal,
choose `shish.wasm` (rename `build/wasi/shish` to that), then
`shish -c 'echo hi'`, or just `shish` for the shell itself. Functions,
subshells, `break` and pipelines between builtins work there.

### What works on WASI

Everything that does not need another process: expansion, arithmetic,
control flow, functions, here-documents, `$(...)` of builtins, pipelines
between builtins and subshells (they run in-process), redirections, and
the file utilities that are compiled in as builtins.

### What does not

- External commands (`sort`, `/bin/sh`, ...) and background jobs (`&`)
  need `fork`: they print `<cmd>: Function not implemented` and give
  status 1. Compile the utilities you need in as [builtins](builtins.md).
- No job control, no terminal handling (`tcgetattr` reports "not a tty"),
  no `/etc/passwd` (`~user` does not expand), a single anonymous user.
- Files are visible only through the host's preopened directories.

## Why put a shell in WebAssembly

- **Documentation and tutorials that actually run.** A shell prompt on a
  docs page, doing real work, with no backend to keep alive.
- **Agent UIs.** A browser-side coding assistant can run its shell
  commands in the tab, against a virtual filesystem, without a container
  per session — see [Agent sandboxes](agents.md).
- **Edge and serverless.** A 184 KB module is a plausible unit of
  deployment where a container image is not.
- **Teaching and testing.** The same shell binary in CI, in the
  container, and in the browser means one behaviour to learn.
