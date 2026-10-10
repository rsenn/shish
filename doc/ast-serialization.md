# AST serialization (JSON and NDJSON)

Status: `src/json.h` implemented (`tests/json_test.c`); `src/ast.h` and `src/ast/` not yet. New module: `src/ast.h` + `src/ast/`, writer in `src/json.h`.

## Context

`debug_list()` / `debug_node()` (`src/debug/debug_{list,node}.c`) print the parse tree as JSON through
the `debug_*` helpers. The helpers work, but they make a second serializer (NDJSON, or any new
format) expensive to add and make `debug_node()` longer than it needs to be:

- **Separator baked into the key.** Callers pass `", bgnd"`; `debug_field()` re-parses a leading
  `,`/space with `str_chr(", ", *s)` on every field. Comma state lives in string literals.
- **`depth` threaded through every call** (`depth`, `depth+1`, `depth+2`, `-1` = "inline"). Four
  helpers re-derive indentation from it (`debug_list`, `debug_sublist`, `debug_subnode`, `debug_newline`).
- **Output is not guaranteed to be JSON.** `debug_str()` escapes `\n`/`\r` only — a `"` or `\` in a
  name or word breaks the document. Control characters and non-UTF-8 bytes are unhandled.
- **Global state.** Output goes through `debug_output`, indent through `debug_nindent`, quote char
  through `debug_quote`, position mode through `debug_emit_loc/range`. All `debug_*` macros vanish
  without `DEBUG_OUTPUT`/`SHPARSE2AST`, so the serializer can't be linked into a release `shish`.
- **Colour and syntax mixed** (`COLOR_*` in `debug_str`, `debug_ulong`, `debug_begin`). The new
  module has no colour at all: it never includes `debug.h`.
- **Position boilerplate repeated** 4× in `debug_node()` (`sh_no_position` + `debug_emit_loc` +
  `debug_emit_range` with the end offset computed per kind).

Goal: a small writer API that tracks commas/indent itself, plus a node layer in which each `case`
of `debug_node()` is 1–3 lines, without changing the JSON that `shparse2ast` / `wasm` emit today.

Consumers today: `sh_parse2ast.c:198` (`debug_list(script, 1)`), `sh_util_wasm.c:243`,
`trace_value.c` (reads `debug_nodes[]` names). Design order (CLAUDE.md): binary size, LOC,
fragmentation, speed.

## Design

Two pieces, both new, neither depending on `debug.h` (so no colour, no `DEBUG_OUTPUT` macros, no
globals):

| piece | where | what |
|---|---|---|
| JSON writer | `src/json.h` (header-only, `static inline`) | commas, indent, string escaping, onto a `buffer*` |
| AST serializer | `src/ast.h`, `src/ast/ast_*.c` | `union node` tree -> JSON / NDJSON via the writer |

The output is plain text: no ANSI escapes, ever. Callers pass the destination buffer
(`fd_out->w`, `debug_output`, a `stralloc`-backed buffer for wasm).

### `src/json.h` — header-only writer

State is five fields. A comma is needed exactly when the previous token was a value or a
closing bracket, so one bit replaces a per-level stack and nesting depth is unbounded; a second
bit marks "a key was just written", whose value takes no separator.

```c
struct json {
  buffer* b;
  int indent;         /* spaces per level; 0 = one line, items separated by ", " */
  int depth;          /* open containers */
  unsigned comma : 1; /* the next item needs a leading "," */
  unsigned key : 1;   /* a key was just written: its value follows without a separator */
  unsigned bare_keys : 1;     /* keys unquoted: kind: ... (JSON5) */
  unsigned single_quotes : 1; /* strings in '...' instead of "..." (JSON5) */
  unsigned hex_numbers : 1;   /* numbers as 0x1f, not 31 (JSON5) */
  unsigned minify : 1;        /* no whitespace at all, whatever indent says */
};

json_init(j, b, indent);

/* containers */
json_open(j, c);      /* c is '{' or '[' */
json_close(j, c);     /* c is '}' or ']'; always on its own line, empty container included */

/* members and scalars */
json_key(j, "k");     /* "k":  -- k is a literal, not escaped */
json_str(j, s, n);    /* \" \\ \n \r \t and other control chars as \u00XX; bytes >= 0x80 pass through */
json_uint(j, v);
json_hex(j, v);       /* "0x1f" as a string, JSON has no hex literal */
json_null(j);

/* key + value in one call; the serializer uses these for members */
json_kstr(j, "k", s); /* NULL s -> null */
json_kuint(j, "k", v);
json_khex(j, "k", v);
```

All are `static inline`; `json_break()` (newline + indent, or one space when `indent == 0`) and
`json_sep()` (what precedes every item) are the internals.

- `json_sep()` is the only place that writes `,` and the line break; every emitter calls it
  first. It writes nothing after a key, and no line break at depth 0, where the caller owns the
  document boundary.
- `json_close()` always breaks the line, so an empty list prints `[` newline `]` as
  `debug_begin()/debug_end()` do today.
- Four independent bits, set by the caller after `json_init` (which clears them). The first three
  together make JSON5; none of them changes layout, commas or `null`.

  | flag | effect |
  |---|---|
  | `bare_keys` | `kind: 'word'` -- the key is written without quotes, so it must be an identifier |
  | `single_quotes` | strings use `'`; the quote is escaped (`\'`), `"` is not |
  | `hex_numbers` | every number is `0x..`: `json_uint()`, `json_kpair()`, and `json_hex()` as a number instead of the string `"0x1f"` |
| `minify` | no whitespace at all: no line breaks or indent (it overrides `indent`), none after `,` or `:`, `{}` and `[]` for empty containers |

- A top-level `json_close()` leaves `comma` clear, so the next document starts clean.
- NDJSON needs no extra API: `json_init(&j, b, 0)`, write one top-level object, then
  `buffer_putc(b, '\n')` (the caller owns document boundaries).
- Size rule: `json_str` is the one function with a loop (~20 lines). `static inline` in a header
  is emitted once per translation unit that calls it, so only `ast_node.c` and `ast_pos.c`
  call it (directly or through `json_kstr`); every other file goes through `ast.h`. Check with
  `nm -S` on `shparse2ast` that `json_str` appears at most twice; if more, move the string path
  into one `.c` file.

### `src/ast.h` and `src/ast/` — serializer

```c
struct ast {
  struct json j;
  unsigned loc : 1;         /* "loc": "file:line:col"          (default on)  */
  unsigned range : 1;       /* "range": [start, end]           (default off) */
  unsigned no_position : 1; /* suppress both; replaces sh_no_position here  */
};

void ast_init(struct ast* a, buffer* b, int indent);
void ast_node(struct ast* a, union node* node);   /* one object   (replaces debug_node) */
void ast_list(struct ast* a, union node* list);   /* one array    (replaces debug_list) */
void ast_tree(struct ast* a, union node* list);   /* whole script: ast_list + trailing newline */
void ast_ndjson(struct ast* a, union node* list); /* one compact object + '\n' per top-level node */
void ast_pos(struct ast* a, const struct location* loc, size_t len);   /* "loc" and/or "range" */

extern const char* const ast_names[];             /* kind -> JSON "kind" name (was debug_nodes[]) */
extern const unsigned ast_names_count;
```

The options that were globals (`debug_emit_loc`, `debug_emit_range`, `sh_no_position`) become
fields of `struct ast`, set by `sh_parse2ast.c` and `sh_util_wasm.c` after `ast_init`.

Helpers in `ast.h`, `static inline`, one to three lines each — they are what keep the `switch`
short:

| helper | replaces |
|---|---|
| `ast_kids(a, "cmds", n)` | `debug_sublist(", cmds", n, depth)`: `json_key` + `ast_list` |
| `ast_kid(a, "left", n)` | `debug_subnode(", left", n, depth)`: skipped when `n` is NULL |
| `ast_bgnd(a, bit)` | `debug_ulong(", bgnd", ...)` |
| `ast_rdir(a, n)` | `if(x->rdir) debug_sublist(", rdir", ...)`, repeated in six cases |

`ast_pos(a, &loc, len)` is a real function: it replaces the `sh_no_position` / `debug_emit_loc` /
`debug_emit_range` triple that appears four times in `debug_node()`; `len` gives the range end.

Files (one function per file, like `src/debug/`):

```
src/json.h
src/ast.h
src/ast/ast_init.c  ast_node.c  ast_list.c  ast_tree.c  ast_ndjson.c  ast_pos.c  ast_names.c
```

`ast_list.c` keeps the empty-placeholder filter (an empty `N_ARGSTR` that only carries quoting
state is dropped, unless it is the only element). `ast_names.c` is the only file that is also
built into the release `shish` (the `trace` module reads kind names through it); the rest is
linked only into `shparse2ast` and the wasm build.

Resulting shape:

```c
case N_IF:
  ast_bgnd(a, n->nif.bgnd);
  ast_rdir(a, n->nif.rdir);
  ast_kids(a, "cmd0", n->nif.cmd0);
  ast_kids(a, "cmd1", n->nif.cmd1);   /* NULL -> omitted */
  ast_kid(a, "test", n->nif.test);
  break;
```

`ast_node` is `json_open(j, '{'); json_kstr(j, "kind", ast_names[n->id]); switch(...); json_close(j, '}')`.

### Rejected

- **Table-driven node shapes (`offsetof` per field).** Would shrink the switch further, but
  `bgnd`, `has_in`, `posix` are bit-fields (`unsigned bgnd : 1`) — no `offsetof`, so the table needs
  accessors and loses the readability it was meant to buy.
- **Callback visitor / SAX interface.** Extra indirection and size for a single consumer shape.
- **A `lib/json/` module with `json.h` in `lib/`.** The writer has one consumer (`src/ast/`), so it
  stays a private header in `src/` until a second one appears.
- **Keeping `debug_*` and adding `json_*` beside it.** Two serializers to keep in sync; the debug
  one would stay non-escaping.
- **`printf`-style formatting / stdio.** Not allowed in this tree (CLAUDE.md "Library calls").

### Compatibility

- Output of `shparse2ast` stays byte-identical in indented mode for ASTs that contain no `"`/`\`
  (diff against the current binary on `tests/*.sh` and `examples/`); the escaping fix is an
  intentional change and gets a `BUGS`-style note + `tests/fixed.sh` case.
- `debug_list`/`debug_node` remain as thin wrappers (`ast_init` on `debug_output`, call
  `ast_list`) until `sh_parse2ast.c`/`sh_util_wasm.c` are ported, then are deleted along with
  `debug_sublist/subnode/field/begin/end/ulong/xlong/str/stralloc/position/range/location` that
  only they use. `debug_str`, `debug_unquoted`, `debug_flags` etc. used by the
  non-AST debug output are untouched.
- CLI: `shparse2ast -j` (indented, today's default), `-n` (NDJSON: one compact object per
  top-level command), `-w N` indent kept, `-q` (quote char) dropped (JSON only allows `"`),
  `-l loc|range|both` kept.

## Migration steps

1. Add `src/json.h`; unit test in `tests/` (compile a small program or run through `shparse2ast`:
   escaping, nesting, comma placement, compact mode, empty containers).
2. Add `src/ast.h` and `src/ast/ast_*.c`; move `debug_nodes[]` to `ast_names.c` as `ast_names[]`
   and update `src/trace/trace_value.c`.
3. Port `sh_parse2ast.c` and `sh_util_wasm.c`; make `debug_list/node` wrappers, then delete them
   and the `debug_*` helpers nothing else uses.
4. Update `BUGS` / `TODO.md` / `fixes/NN-*.patch` / `tests/fixed.sh` for the escaping fix; update
   `doc/debug-output.md` if it documents `shparse2ast` output.
5. Register the new files: `CMakeLists.txt` (explicit `LIBSHELL_SOURCES` lists near the `debug_*`
   entries; `ast_names.c` in every build, the rest with `shparse2ast`/wasm) and `src/Makefile.in`.

## Verification

- Before/after `size` on a `MinSizeRel` build of `shish` and `shparse2ast` (design metric 1);
  the writer must not grow `shish` (the AST code is only linked into `shparse2ast`/wasm).
- Byte-for-byte diff of old vs new `shparse2ast` output over `tests/*.sh`, `examples/*` (indented
  and `-w0`), excluding inputs with `"`/`\`.
- New escaping test: a script with `echo "a\"b"` and a function named with a backslash round-trips
  through `jq .` (or `python3 -m json.tool`) — fails before, passes after.
- NDJSON: `shparse2ast -n script | while read l; do echo "$l" | jq -e . >/dev/null; done`; line
  count equals top-level command count.
- `nm -S shparse2ast | grep json_`: no function emitted more than twice.
- `grep -rn "COLOR_\|debug.h" src/ast src/ast.h src/json.h` is empty (no colour, no debug macros).
- `ctest` (tests/*.sh via shish) plus the new json unit test; wasm build via `cfg-wasm` still links.
