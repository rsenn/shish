# Filter infrastructure

How a builtin joins an in-process pipeline (TODO.md, Goal 13), what the shared
code in `src/builtin/builtin_filter.[ch]` does for it, and what is still to do.

- [1. Why](#1-why)
- [2. The pieces](#2-the-pieces)
- [3. Writing a filter](#3-writing-a-filter)
- [4. Rules a filter must follow](#4-rules-a-filter-must-follow)
- [5. Where each builtin stands](#5-where-each-builtin-stands)
- [6. Open work](#6-open-work)
- [7. Assessment: shared code for head, tail, uniq, cut, paste, nl, tr](#7-assessment-shared-code-for-head-tail-uniq-cut-paste-nl-tr)

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

There are no hand-written overrides: every filter goes through the same
`filter_open`/`filter_close`, and `step` is the only way to produce data.

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
| `compress` | declarative | libarchive write side, algorithm from `argv[0]`; also has file mode (`f` -> `f.gz`/`.bz2`/...) and `-d` delegation to `uncompress` |
| `uncompress` | declarative | libarchive read side, lends the input buffer in place and reads libarchive's decode block directly; `-f` copies non-compressed input |
| `sed` | declarative | runs the script to completion on the first `step`, then hands out its captured output as one unit; declines file operands and `w` targets |

`filter_ops.status` feeds `filter_run`'s exit code and, under `set -o pipefail`, the
pipeline's status for chained members.

## 6. Open work

Done:

- table rows for `gzip`, `zcat` and aliases carry `&compress_filter` / `&uncompress_filter`;
- `compress` picks the libarchive filter and file suffix from `argv[0]`
  (`gzip .gz`, `bzip2`/`lbzip2 .bz2`, `xz .xz`, `zstd .zst`, `lz .lz`);
- `sed` is declarative too; the `open`/`read`/`close` overrides and `fd_filter`'s `read`
  fallback are gone, so `step` is the only data path;
- an external last command inherits the real fd 0, so the chain's in-process buffer is
  invisible to it (`cat f | sort` used to print nothing). It is fed by a pump instead: the chain's last link is drained by a
  detached child (double fork, nothing to reap) into a real pipe that becomes the
  command's stdin (`pipeline_chain_pump`, `eval_pipeline.c`);
- `filter_drain` flushes only when the next read would wait (the active source has
  nothing buffered): an mmapped file goes out in full buffers, a slow pipe or a
  terminal stays live (`cat` on 4 MB: 4 049 writes instead of one per line);
- `set -o pipefail` (no letter): the rightmost non-zero member status, for forked and
  chained members, including a chain that ran in a pump child (it sends one status byte
  per stage back through a pipe before it closes the data pipe; a last command that quits
  early leaves them 0), and for backgrounded pipelines (`job->pipefail`, applied in
  `job_wait()`, so `wait $!` sees it). `filter_ops.status` feeds it.

In order:

1. **Filters still on the TODO list** (`head uniq paste cut tr nl tail`; none exists as a
   builtin yet): each should be one `filter_ops` plus a step function.

## 7. Assessment: shared code for head, tail, uniq, cut, paste, nl, tr

Goal: each `builtin_<name>.c` holds only its own decision (what to keep, change or count),
about 30-70 lines, because everything mechanical lives once in `builtin_filter.[hc]` or `lib/`.
Sizes are from the TODO.md Goal 16 table (`head` ~70, `uniq` ~110, `cut` ~140, `tail` ~130,
`nl` ~170, `tr` ~200, `paste` ~90 lines) before extraction.

### What each utility does with its input

| | reads | keeps state | writes | its own logic |
|---|---|---|---|---|
| `head` | lines or bytes, stops early | count | prefix of the input | `-n N`, `-c N` |
| `tail` | lines or bytes, all of it | last N lines (ring) or a seek | suffix | `-n N`, `-n +N`, `-c`, `-f` (no chain) |
| `uniq` | lines | previous line, run count | a line, optionally with a count | `-c -d -u -f N -s N` |
| `cut` | lines | none | selected bytes/chars/fields of a line | list `1,3-5,7-`, `-d`, `-s` |
| `paste` | N inputs in parallel, lines | none | joined lines | `-d list`, `-s` |
| `nl` | lines | line number, section | line with a number prefix | sections, `-b/-h/-f`, `-n -w -s -v -i` |
| `tr` | bytes | last byte (`-s`) | mapped bytes | set parser, `-c -d -s` |

### Common needs, and where each belongs

| # | need | used by | today | proposal |
|---|---|---|---|---|
| 1 | **a line of any length**, zero-copy when it lies inside the buffered window | head tail uniq cut paste nl (and grep, sed) | `filter_in_get(buf, 1024, "\n")` splits long lines (BUGS: `grep-splits-lines-longer-than-1kib`) | `ssize_t filter_in_line(in, const char** p, int* had_nl)` in `builtin_filter.c`: `memchr` in the `peek` window and return a pointer into it (newline included), or spill to a growable buffer in `filter_in` only when the line straddles two windows |
| 2 | **an output unit built from pieces** (prefix + line, joined fields, mapped bytes) | uniq -c, cut, paste, nl, tr | hand-rolled `linebuf[]` with a fixed size (`cat`, `grep`) | use `stralloc` in the ctx: reset it at the top of `step`, return `sa.s`/`sa.len`. No new API; document the pattern and drop the fixed `linebuf` arrays |
| 3 | **take/skip N lines or bytes** without touching each line | head, tail +N, tail -c, head -c | none | `size_t filter_in_peek_lines(in, &p, max_lines)` (longest prefix of the window with at most `max_lines` newlines; `head` returns it as one unit) and `filter_in_skip_lines(in, n)` |
| 4 | **numeric option arguments** (`-n 5`, `-5`, `+5`, byte counts with `k`/`m`) and the "invalid number of lines" message | head, tail, uniq -f/-s, nl -w/-v/-i, cut -f | `scan_ulong` plus ad hoc checks | `int filter_opt_count(const char* s, unsigned long* out, int allow_plus)` returning -1 on junk/overflow; the obsolescent `-5` form falls out of declaring the digits in `opts` and letting `option` accumulate them (`filter_opt_digit(&n, ch)`) |
| 5 | **several inputs read in parallel** | paste | `filter_in` cycles operands one after another | `struct filter_in` is ~4.2 KiB because of its `rbuf[4096]`, so an array is wasteful: allocate `rbuf` only for a non-mmapped source, then paste can keep `n` of them. `-` repeated means one shared stdin, taken round-robin |
| 6 | **the last N lines** | tail | none | a small `struct line_ring` (byte arena plus offsets, drops the oldest whole line when full) in `lib/`; for a seekable file scan backwards through the mmap and copy nothing |
| 7 | **byte sets** (`[:alpha:]`, ranges, complement) | tr (sed `y`, dfa already have one) | `class_mark(unsigned char set[32], ...)` inside `text/dfa/dfa_bracket.c` | move `class_mark` and the 256-bit set operations to `lib/byteset.[hc]`, used by dfa and tr; `tr` adds only its `string1 string2` grammar (escapes, `a-z`, `[x*n]`, `[=e=]`) |
| 8 | **field skipping** | uniq -f, cut -f | none | `text_skip_fields(p, n, k)` and `text_skip_chars(p, n, k)`, 10 lines each in `lib/` (also what a later `sort -k`, `join`, `comm` need) |
| 9 | **number formatting with padding** | nl, cat -n, wc | `fmt_ulong` plus hand padding in each | `fmt_ulong_pad(dst, v, width, ' '/'0', left/right)` next to `fmt_ulong0` in `lib/fmt` |
| 10 | **regex on a line** | nl -b pBRE, grep | `dfa_compile`/`dfa_test` open-coded in `grep` | none needed; `nl` copies the six lines. If a third user appears, add `filter_line_regex` |
| 11 | **characters vs bytes** | cut -c, tr, uniq -s | `wc` counts bytes, `lib/utf8.h` has `u8len` | one seam: `size_t text_charlen(const char* p, size_t n)` (bytes now, `u8len` later), so every filter changes together |
| 12 | **builtin boilerplate**: three lines in the `.c`, an `extern`, a table row, a `Builtins.cmake` entry, a help string | every new builtin | copied by hand | a `FILTER_BUILTIN(name)` macro in `builtin_filter.h` generating `builtin_<name>` and `<name>_filter` from `<name>_ops` |
| 13 | **tests**: each utility needs a direct run and a chained run of the same case | every new builtin | one hand-written test per form | `assert_filter "cmd" "input" "expected"` in `tests/common.sh`, running `printf input | cmd`, `cmd < file`, and `cat file | cmd | cat` |

### Effect on the builtins

With 1-3 in place, `head` is an option callback for `-n/-c` and a step that returns
`filter_in_peek_lines()`; `cut` is the list parser plus a step that appends selected pieces
to a `stralloc`; `uniq` is a step that compares `filter_in_line()` against the saved previous
line. Expected sizes after extraction: `head` ~30, `paste` ~50, `uniq` ~60, `cut` ~90 (the list
parser is the bulk and is `cut`'s own), `tail` ~70, `nl` ~110, `tr` ~90 (the set grammar).

### Build order

1. `filter_in_line` (1): also fixes the grep bug and lets `grep`/`sed` drop their fixed line buffers.
2. `filter_in_peek_lines` / `filter_in_skip_lines` (3) and `filter_opt_count` (4), then `head`.
3. `filter_in` with a lazy `rbuf` (5) and `fmt_ulong_pad` (9), then `paste` and `nl`.
4. `text_skip_*` (8), then `uniq` and `cut`.
5. `line_ring` (6), then `tail`.
6. `lib/byteset` (7) extracted from `text/dfa`, then `tr`.
7. `FILTER_BUILTIN` (12) and `assert_filter` (13) before the first new builtin, so all of them use them.

### Risks and decisions for you

- **Bytes or characters.** The proposal keeps everything byte-based like `wc`, behind `text_charlen`.
  POSIX wants characters for `cut -c` and `tr` in a UTF-8 locale; say if you want that first.
- **Missing final newline.** Each file's last line counts as a line, and the output gets the newline
  back only for utilities that rewrite the line (`cut`, `paste`, `nl`, `uniq`); `head` and `tail` copy
  bytes exactly.
- **`tail -f`, `more`** cannot chain and stay direct-run only.
