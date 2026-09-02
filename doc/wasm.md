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

That produces `build/emscripten/shish.js` and `shish.wasm`. The linker
flags above already pass what the glue needs:

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

## WASI (server-side runtimes)

```sh
CC=clang CXX=clang++ CFLAGS=--target=wasm32 CXXFLAGS=--target=wasm32 LDFLAGS=--target=wasm32 \
  cmake -S . -B build/wasm32-clang \
  -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_SYSTEM_PROCESSOR=wasm32 \
  -DENABLE_SHARED=OFF -DENABLE_PIC=FALSE
cmake --build build/wasm32-clang -j
```

This targets a plain `wasm32` module rather than Emscripten's JS
environment — for wasmtime, wasmer, or an embedder that provides its own
WASI imports. The same fork restriction applies.

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
