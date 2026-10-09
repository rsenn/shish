# WASM playground: shformat + shparse2ast modules, linker-bloat fix, AST schema

## Context

The gh-pages playground currently only runs `shish.wasm` (execute). The user
wants to extend it with a shell-script formatter/syntax-checker and an
AST explorer + refactoring demo built on `shparse2ast`, both compiled to
WASM. While investigating, the user asked whether these two parser-only
tools are linker-optimized — they are not: both `shformat` and
`shparse2ast` link in the *entire* interpreter (eval/exec/builtin/job/redir,
522 symbols) despite calling none of it. That bloat needs fixing before
shipping two more multi-hundred-KB WASM modules to a browser.

Separately, the JSON AST currently labels nodes with shish's internal C
enum names (`N_SIMPLECMD`, `N_ARGPARAM`, ...), which isn't intelligible to
a JS consumer and isn't a stable public schema. The user wants node types
named after the POSIX Shell Command Language grammar (the grammar shish
already targets) instead.

## Root cause of the linker bloat (confirmed via `-Wl,-Map`)

`src/parse/parse_simpletok.c.o` — the tokenizer, used by every consumer of
the parser including `shparse2ast`/`shformat` — calls `prompt_show()`
(PS2 continuation-prompt display). That single reference chains:

```
parse_simpletok.c.o -> prompt_show.c.o -> term_init.c.o -> term_read.c.o
  -> job_init.c.o, builtin_trap.c.o -> eval_tree.c.o -> exec_*/builtin_table
```

pulling the whole interpreter into any binary that links the tokenizer,
whether or not it ever runs a command. This is the entire explanation for
the ~522 unwanted symbols in `shparse2ast`/`shformat`.

### Fix: break the static reference with a function-pointer hook

- New file `src/parse/parse_prompt_hook.c`: defines
  `void (*parse_prompt_hook)(struct parser*) = 0;` (name/signature to match
  whatever `prompt_show` currently takes).
- `src/parse.h`: declare `extern void (*parse_prompt_hook)(...);`.
- `src/parse/parse_simpletok.c`: replace the direct `prompt_show(...)` call
  with `if(parse_prompt_hook) parse_prompt_hook(...);`.
- `src/sh/sh_main.c` (the only real interactive shell entry point): set
  `parse_prompt_hook = prompt_show;` once at startup, before the main loop.
- `src/sh/sh_fmt.c` and `src/sh/sh_parse2ast.c`: do NOT set the hook — they
  never reference `prompt_show` by name, so the linker drops
  `prompt_show.c.o` and everything it pulls (`term_*`, `job_init.c.o`,
  `builtin_trap.c.o`, `eval_tree.c.o`, and therefore `exec_*`/`builtin_*`)
  for those two targets.

This is a single small new file plus three call-site edits — no new
abstraction beyond the one indirection needed to cut the reference.

### Verification

- Native: rebuild `build/x86_64-linux-gnu`, re-run
  `nm shparse2ast | grep " T " | grep -iE "exec_|eval_|builtin_|job_|redir_"`
  — should now be empty (or reduced to only genuinely-referenced misc
  helpers, if any remain investigate further before considering this done).
  Compare `size shish shformat shparse2ast` before/after.
- WASM: rebuild `build/emscripten-all`, compare `.wasm` code-section size
  via `wasm-objdump -h` before/after; expect a substantial drop for
  `shformat`/`shparse2ast` (they were only 2-8% smaller than `shish`
  before the fix).
- Run full `ctest -j4` in the native build to confirm zero regressions
  (the hook must still make `shish` show PS2 prompts correctly —
  interactively test `shish` with an unterminated `if` and confirm the
  continuation prompt still appears).
- Add the fix as a numbered `fixes/NNN-...patch` per repo convention, and
  a case in `tests/fixed.sh` if a reasonable one exists (e.g. asserting
  `nm`/`size` shows no interpreter symbols in `shformat` — likely only
  practical as a build-time/manual check rather than a `tests/*.sh` case;
  if so, note in `BUGS`/`TODO.md` instead of forcing an artificial test).

## AST JSON schema: POSIX grammar rule names

`src/debug/debug_node.c:17-20` has one flat string table indexed by the
`N_*` enum, currently emitted verbatim as the `"kind"` field. Replace the
table's strings with POSIX Shell Command Language (§2.10) rule names:

| enum | new `kind` string |
|---|---|
| `N_SIMPLECMD` | `simple_command` |
| `N_PIPELINE` | `pipeline` |
| `N_AND` / `N_OR` | `and_or` |
| `N_NOT` | `not` |
| `N_LIST` | `list` |
| `N_SUBSHELL` | `subshell` |
| `N_BRACEGROUP` | `brace_group` |
| `N_FOR` | `for_clause` |
| `N_CASE` | `case_clause` |
| `N_CASENODE` | `case_item` |
| `N_IF` | `if_clause` |
| `N_WHILE` | `while_clause` |
| `N_UNTIL` | `until_clause` |
| `N_FUNCTION` | `function_definition` |
| `N_ARG` | `word` |
| `N_ASSIGN` | `assignment` |
| `N_REDIR` | `redirect` |
| `N_ARGSTR` | `string` |
| `N_ARGCMD` | `command_substitution` |
| `N_ARGPARAM` | `parameter_expansion` |
| `N_ARGARITH` | `arithmetic_expansion` |
| `A_NUM` / `A_PAREN` | keep as-is (arithmetic sub-nodes, not grammar rules) |

This is a one-table edit — no structural change to `debug_node.c`'s logic.
Keep the field name `"kind"` (already established/used by earlier work
this session — confirm no external consumer besides the new playground
depends on the old `N_*` strings; none do yet, since the playground AST
tab doesn't exist).

## Sequencing

Implement in this order, each step verified before starting the next:

1. Linker untangling (`parse_prompt_hook`, see above) — land as its own
   fix/commit, verified with `nm`/`size` and `ctest` on native
   `shformat`/`shparse2ast`, before touching WASM or the playground.
2. Build `shutil.wasm` (the merged module, see below).
3. Wire it into the gh-pages playground, **Format tab first**: a "Format"
   demo using deliberately badly-formatted example shell scripts (mixed
   indentation, inconsistent spacing around `;`/`|`/`&&`, cuddled
   `then`/`do`, etc.) so the before/after is obviously worth showing.
4. Write the `shutil-js` wrapper API in full (format + parse + visitor
   utilities + splice print-back).
5. Add the third playground demo: collapsible AST tree explorer, the
   variable-rename visitor, and re-emitting the refactored script via
   `spliceEdits`.

## One combined WASM module instead of two

`shformat` and `shparse2ast` both do exactly one parse and then only
differ in what they do with the tree (print reformatted shell text vs.
dump JSON). Once the linker fix lands, both are just "parser + one small
printer" — small enough, and similar enough, that shipping two separate
`.wasm` files to a browser (two fetches, two copies of the deduplicated
parser code) is wasted weight for the playground. Merge them into a
single **`shutil` WASM-only target** (native `shformat`/`shparse2ast`
stay as they are — separate, documented CLI executables; nothing about
their existing behavior, flags, or tests changes):

- New file `src/sh/sh_util_wasm.c`, built only when `EMSCRIPTEN` and
  `BUILD_SHUTIL_WASM=ON` (new CMake option, off by default like
  `BUILD_SHPARSE2AST`). No `main()` / no `callMain` usage at all — this
  is a library-style module, not a CLI re-skinned for the browser.
- Two `EMSCRIPTEN_KEEPALIVE` exported functions operating on in-memory
  strings (reusing the same parse setup `sh_parse2ast.c` already does,
  minus its CLI arg handling):
  - `char* shutil_format(const char* src)` — parse + reuse `shformat`'s
    existing tree-printer, return the formatted text (or an error
    string prefixed in a way the JS wrapper can detect — match whatever
    convention `sh_fmt.c` already uses for reporting a syntax error, e.g.
    a leading `path:line:col:` message on stderr today; decide the exact
    in/out-of-band error signal when implementing, based on what
    `sh_fmt.c`'s current error path actually does).
  - `char* shutil_parse_ast(const char* src)` — parse + reuse
    `debug_list`/`debug_node`'s existing JSON emission, return the JSON
    string.
  - Both allocate their return buffer with `malloc` (freed from JS via a
    matching `shutil_free(char*)` export, the standard Emscripten
    string-return pattern) rather than writing to a fixed fd/buffer.
- CMake: `-sMODULARIZE=1 -sEXPORT_NAME=createShutil
  -sEXPORTED_FUNCTIONS=['_shutil_format','_shutil_parse_ast','_shutil_free','_malloc','_free']
  -sEXPORTED_RUNTIME_METHODS=['cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8']`
  (no `INVOKE_RUN=0`/`callMain` flags needed since there's no `main`).
- This only works cleanly because of the linker fix above: without it,
  `shutil.wasm` would still drag in the interpreter, defeating the point
  of merging.

## JS API around the two WASM modules (`shish.wasm`, `shutil.wasm`)

Two small, purpose-specific JS wrapper modules (vanilla JS, no framework,
matching the site's existing zero-dependency style), each lazy-loading
its own `.wasm`:

- **`shish-run.js`** — wraps `shish.wasm`. `run(script) -> {stdout, stderr,
  exitCode}`, using `Module.callMain(['-c', script])` (existing convention)
  with captured FS/stdout hooks. Unchanged from `shish`'s current
  non-modularized embed pattern otherwise.
- **`shutil-js`** — wraps `shutil.wasm` via `cwrap`:
  - `format(source) -> {formatted, ok, error}` — calls `shutil_format`.
    Also serves as a syntax checker (`ok: false` = syntax error).
  - `parse(source) -> astJson` — calls `shutil_parse_ast`, `JSON.parse`s
    the result.
  - Visitor utilities operating purely in JS on the returned JSON:
    - a generic recursive walker,
    - the variable-rename demo: find the declaring `assignment`/`string`
      node and every `parameter_expansion` referencing the name (including
      recursion into `.word`), using the already-confirmed `loc`
      conventions (`parameter_expansion.loc` = end-of-name position;
      assignment `string.loc` = start-of-string position),
    - `spliceEdits(source, edits)` — applies the collected `(loc, oldLen,
      replacement)` edits directly to the **original source text**, so
      output preserves the user's exact formatting/comments outside the
      edited spans. This is the "print back a shell script" capability
      for now — no C-side AST→text regeneration.

**Deferred, not implemented now:** a WASM entry point that accepts a
JS-modified JSON tree, rebuilds the C `union node` tree, and reuses
shformat's printer to emit canonical (reformatted) text. Add this as a
`TODO.md` item under a "later" section: real AST-edit-and-reprint pipeline,
noting it requires a JSON→AST deserializer in C and normalizes away
original formatting, so splice-based editing remains preferable for a
refactoring tool even after it exists.

## Playground UI (`tools/site/play.html`, `tools/site/build.js`)

- `build.js`'s `buildPlayground()` (~lines 257-266): extend to also copy
  `shutil.js/.wasm` into `docs/assets/`.
- `play.html`: add Run / Format / AST-explorer tabs; each WASM module
  loads lazily on first use of its tab (don't fetch 3 modules upfront).
  Ship Format first (step 3 of Sequencing), AST-explorer afterward
  (step 5) — they can land as separate commits.
- Format tab: a small picker of 2-3 deliberately ugly example scripts
  (inconsistent indent, `if test -f x;then` cuddled keywords, stray
  trailing whitespace, mixed tabs/spaces, a one-liner pipeline crammed
  onto one line) plus a free-text textarea, "Format" button, side-by-side
  before/after.
- AST tab: collapsible JSON tree renderer (vanilla JS, consistent with
  the rest of the site), plus the rename-demo control (pick a variable
  name, click rename, see the spliced-output script re-run through
  `format`'s syntax-check to confirm it's still valid).

## Documentation

- `doc/wasm.md`: document the new `shutil` WASM-only target (its
  `BUILD_SHUTIL_WASM` option, exported functions, and that it has no
  native/CLI counterpart — `shformat`/`shparse2ast` remain the CLI tools).
- `TODO.md`: add the deferred "full AST→text regeneration in C" item.
- `BUGS`: any bug found in `shformat`'s printer while reusing it for
  `shutil_format` (e.g. the known `shformat -c` CLI bug, which is
  separate from the in-memory `shutil_format` path and doesn't need
  fixing just to build `shutil` — note it as still-open if untouched).

## Verification

1. Native build + `ctest -j4`: zero regressions from the prompt-hook
   change; PS2 prompt still works interactively in `shish`.
2. `nm`/`size` diff on native `shparse2ast`/`shformat` before/after the
   linker fix — interpreter symbols gone. Land and verify this stage on
   its own before starting the `shutil` merge.
3. WASM build of `shutil` (`build/emscripten-all` or equivalent), confirm
   via `wasm-objdump -h`/`nm` that it carries no eval/exec/builtin/job
   symbols, and compare its size against a same-flags un-merged build to
   confirm the merge actually saved size over shipping two modules.
4. Serve `docs/` locally (or open `play.html` directly) and manually
   exercise all tabs in a browser: Run a script (`shish.wasm`), Format a
   script, parse a script into the AST tree view, run the rename demo
   end-to-end and confirm the spliced output is valid shell that still
   parses (feed it back through `format`'s syntax-check path).
5. Rebuild the site (`qjsm tools/site/build.js docs`) and confirm no
   unrelated diffs before publishing.
