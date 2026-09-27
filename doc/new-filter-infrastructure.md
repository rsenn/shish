# Filter infrastructure

How a builtin joins an in-process pipeline (TODO.md, Goal 13), what the shared
code in `src/builtin/builtin_filter.[ch]` does for it, and what is still to do.

- [1. Why](#1-why)
- [2. The pieces](#2-the-pieces)
- [3. Writing a filter](#3-writing-a-filter)
- [4. Rules a filter must follow](#4-rules-a-filter-must-follow)
- [5. Where each builtin stands](#5-where-each-builtin-stands)
- [6. Open work](#6-open-work)

---

## 1. Why

A pipeline made only of filter-capable builtins runs in one process: each stage's
output is a `buffer` that the next stage reads, with no `fork()` and no pipe.
Before this rework every filter hand-wrote the same lifecycle (`open`, `read`,
`status`, `close`, an option loop, a `struct filter_out`) and copied every byte
about five times on the way through (`zcat`: input, libarchive, `raw_out`,
`filter_out`, consumer buffer).

Goals of the rework:

- a filter states *what* it is (options, context size, one step function) and the
  framework does the plumbing, so a builtin shrinks to its actual algorithm;
- data moves as pointers, not copies;
- no fixed-size scratch buffer that a large unit can overflow.

## 2. The pieces

| piece | file | job |
|---|---|---|
| `struct filter_in` | `builtin_filter.h` | the input side: file operands in order, `-` or none means stdin/upstream |
| `filter_in_get` / `filter_in_peek` + `filter_in_skip` | `builtin_filter.c` | copying read (with delimiters) / zero-copy read of the buffered bytes |
| `filter_step_fn` | `builtin_filter.h` | `int step(ctx, &unit, &len)`: next unit as a pointer, 0 at EOF or error |
| `struct filter_ops` | `builtin_filter.h` | what a builtin declares (section 3) |
| `filter_init/open/status/close/run` | `builtin_filter.c` | generic lifecycle behind a `filter_ops` |
| `filter_drain` | `builtin_filter.c` | direct run: `step` to completion, write and flush each unit |
| `fd_filter`, `buffer_filter_init` | `src/fd/fd_filter.c` | turns a `filter_ops` + ctx into a read `buffer` for the next stage |
| chain setup | `src/eval/eval_pipeline.c` | opens every candidate stage or none, links them, rolls back on decline |

### Data flow

```
file / stdin --filter_in_peek--> step() --unit pointer--> consumer buffer --> next stage
   (mmap or fd buffer,            (owns the                (b->x is pointed at the
    lent in place)                 unit's storage)          unit: no memcpy)
```

`fd_filter_op` calls `step()` and points the consumer's `buffer` window at the
returned unit (`b->x = unit; return len`); the buffer's own storage is kept in
`fd_filter_state.own` and restored before the next refill. Filters that only have
a `read` op still work: it is called with the buffer's real storage.

## 3. Writing a filter

Fill in the declarative half of `struct filter_ops`; the framework zero-allocates
a context of `size` bytes, parses options, sets up `filter_in`, and cleans up.

```c
struct cat { struct filter_in in; int number_lines; /* ... */ };  /* filter_in FIRST */

const struct filter_ops cat_ops = {.opts = "nb", .size = sizeof(struct cat),
                                   .option = cat_option, .setup = cat_setup, .step = cat_step};
const struct builtin_filter cat_filter = {&cat_ops};

int builtin_cat(int argc, char* argv[]) { return filter_run(&cat_ops, argc, argv, fd_out->w); }
```

Then register `&cat_filter` as the last field of the builtin's row in
`src/builtin/builtin_table.c`.

| field | meaning |
|---|---|
| `opts` | `shell_getopt()` string; NULL: no options |
| `size` | `sizeof(ctx)`; the ctx is zeroed and starts with a `struct filter_in` |
| `option(ctx, ch)` | one parsed option; `-1` rejects it (usage error) |
| `setup(ctx)` | after options and operands are known. `0` ok, `1` valid but not streamable, `-1` usage error |
| `step` | next unit as `(pointer, length)`; valid until the next call |
| `status(ctx)` | exit status once `step` returned 0; NULL: `in.had_error` |
| `finish(ctx)` | free what `setup`/`step` allocated (must tolerate a half-initialised ctx) |

Hand-written overrides, each used instead of its default when non-NULL: `open`,
`read` (instead of `step`), `close`. `sed` still uses all three.

### `setup` return values

- `0`: fine.
- `1`: the arguments are valid but this run cannot stream (`grep -c`, `grep -q`, a bad
  pattern, no pattern, `gzip -d`). `filter_open` declines with NULL and the pipeline
  falls back to `fork()` + pipe, re-running the same argv through the builtin's normal
  entry point, which reports the problem. `filter_run` treats it as success.
- `-1`: usage error. `filter_run` prints `invalid option` and returns 1.

## 4. Rules a filter must follow

1. **Declining is silent and consumes nothing.** `setup` returning 1 or -1 must not
   print or read input; otherwise the fallback re-run sees a truncated stream.
2. **A unit stays valid until the next `step` call.** Point at storage in the ctx
   (or in a library that owns it, e.g. libarchive's decode block), never a stack
   local. The consumer keeps reading from it in place.
3. **Never return a zero-length unit as success** if more may follow; `fd_filter_op`
   skips them, but a zero would otherwise read as EOF.
4. **`struct filter_in in` is the first member** of the ctx. The framework casts the
   ctx to `struct filter_in *`.
5. **Only `buffer_feed`-based readers may read a filter buffer.** Its window can point
   at foreign memory, so `buffer_prefetch` (used only by `src/source/source_peekn.c`)
   must not be applied to one.
6. **A data error found in `setup`** (unreadable input) is reported there, sets
   `had_error`, and lets `step` return EOF immediately; it is not a decline.

## 5. Where each builtin stands

| builtin | ops | notes |
|---|---|---|
| `cat` | declarative | 83 lines |
| `grep` (filter half) | declarative | `-c`/`-q`/no pattern/bad pattern decline via `setup` = 1; the direct `builtin_grep` is separate |
| `compress` | declarative | libarchive write side; also has file mode (`f` -> `f.gz`) and `-d` delegation to `uncompress` |
| `uncompress` | declarative | libarchive read side, lends the input buffer in place and reads libarchive's decode block directly; `-f` copies non-compressed input |
| `sed` | `open`/`read`/`close` overrides | uses its own output mechanism; candidate for `step` |

Known dead surface: nothing outside the builtins calls `filter_ops.status`; the chain
never consults a filter's exit status.

## 6. Open work

In order:

1. **Wire the table rows.** `gzip`, `zcat` and their aliases in
   `src/builtin/builtin_table.c` still have no `&compress_filter` /
   `&uncompress_filter`, so those stages always fork. One field per row.
2. **Pick the algorithm from `argv[0]`.** `bzip2`, `xz`, `zstd`, `lz`, `lbzip2` all run
   `builtin_compress`, and `compress_writer_new()` always adds the gzip filter. Map the
   command name to the libarchive filter (and to a suffix for file mode: `.bz2`, `.xz`,
   `.zst`, ...); `setup` is the place, since it already sees `in.errargv[0]`. Decompress
   aliases (`bzcat`, `xzcat`, `zstdcat`) already work through `support_filter_all`.
3. **Convert `sed` to `step`.** Needs its pattern-space output handed out as units
   instead of copied into a `read` buffer.
4. **Use the chain exit status,** or drop the `status` op.
5. **Filters still on the TODO list** (`head uniq paste cut tr nl tail`): each should be
   one `filter_ops` plus a step function.
6. **Direct-mode I/O.** `filter_drain` flushes after every unit; batching for
   non-interactive output would cut syscalls.
