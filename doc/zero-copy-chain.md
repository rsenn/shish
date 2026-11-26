# Zero-copy filter chain: draft and open questions

Status: **draft, not implemented.** This file records the idea as understood so far
and the questions that still decide the design. Nothing here is a commitment.

## 1. The idea (as stated)

- A proof of concept built on the path library (`lib/path.h`).
- A **directory walker** (the core of `find`, `ls -R`) feeding a **path transformer**
  (`realpath`, `readlink`, ...).
- The walker assembles its output **directly in the buffer of a filter further down
  the chain**, ideally the last one, which is redirected to an **mmaped file**.
- The whole chain is then **zero-copy and zero-allocating**.
- The idea goes further than this summary; the remaining aspects are still to be elicited.
- Stages could be **fibers** (as in the Windows API) -- explicitly hypothetical.
- Goal of the PoC: **prove a zero-copy chain.**

## 2. What exists today

| Piece | Where | Behaviour |
|---|---|---|
| Filter contract | `src/filter.h` | `step(ctx, &unit, &len)` returns a pointer valid until the next `step()`; the consumer's buffer points straight at it. |
| Chain link | `src/fd/fd_filter.c` | A `buffer` whose read op (`fd_filter_op`) calls upstream `step()` and sets `b->x` to the unit. |
| Normal builtin | `fd_out->w` / `fd_in->r` | Writes by **push**, reads by **pull**. The `buffer` is where the two meet. |
| Path library | `lib/path.h` | `path_realpath`, `path_canonicalize`, `path_readlink` write into a `stralloc*` that they grow themselves. |
| Input helper | `src/filter.h` | `filter_in_peek()` / `filter_in_skip()` give zero-copy reads of the upstream window. |

## 3. Interpretation

### 3.1 Where the push/pull mismatch lies

- A read stage already sees the producer's memory without a copy: `b->x` is repointed at the unit.
- A producer cannot write **into** the consumer's buffer. The existing mechanism goes only half the way.
- A stage that is naturally push (recursive directory descent) has no way into a chain
  that is pull-only (`step()`), unless it is rewritten as a resumable state machine
  (explicit directory stack, one path per call) or runs as a fiber.
- The `path_*` functions take a `stralloc*`, so their destination is always their own growable buffer,
  never a window owned by a downstream stage.

### 3.2 Proposed shape: shared window, switch as the op

- One `buffer*` is both the producer's `fd_out->w` and the consumer's `fd_in->r`.
- The producer calls `buffer_put()` as any builtin does.
- When the window is full, the buffer's `op` does not call `write(2)`; it **switches to the consumer**.
- When the consumer's window is empty, its `op` **switches back**. Nobody copies.
- This needs a "borrowed window" mode of `buffer`: both sides address the same `x`/`p`/`n`,
  and `op` is the hand-over.
- A fiber (`ucontext`, or hand-written stack switch) is one implementation of the hand-over;
  a plain call into the other side's resumable function is the stackless alternative.
- With one fiber running at a time, a shared window needs no locking.

### 3.3 The last stage

- The final sink is a file mapped with `mmap`; its mapping could be the window for the whole chain.
- Then every stage writes and reads in place, and the output costs no extra copy.
- Growth would be `ftruncate` + `mremap`, or a pre-sized file.

### 3.4 Path API fit (`doc/path-lib.md`)

- **Write side fits.** Writers are `size_t f(char* out, size_t cap, ...)`: they return the length they
  *need* and write only if it fits. Pointing `out` at the free space of the shared window makes this a
  reserve/commit protocol with no allocation: `n <= cap` means the bytes are already in place, commit `n`;
  `n > cap` means grow or flush the window and call again. `f(0, 0, ...)` is the sizing call.
- **Read side needs a whole record.** A transform (`realpath`, `canon`, `readlink`) cannot start on a
  fragment: it needs at least one complete path. The read unit is therefore a delimited record
  (NUL or `\n`), found by peeking to the delimiter, as `filter_in_line()` does for lines.
  Consequences:
  - a producer must never hand over half a path; a flush at an arbitrary byte is not allowed;
  - the window must hold at least one `PATH_MAX` record plus its output;
  - a record longer than the window is the failure case.
- **In place or ahead.** Transforms that only shrink (`canon`, `dirname`, `basename`) can run in place
  (write pointer never ahead of read pointer). `realpath` and `readlink` may grow a path, so they write
  ahead of the input: the window is an append-only arena or a ring, not an overwritten buffer.

## 4. Design metrics check (from `CLAUDE.md`)

1. **Binary size** -- fibers add a context-switch routine per platform; measure with `size` on `MinSizeRel` before and after.
2. **Lines / complexity** -- a borrowed-window mode in `buffer` plus a hand-over op should be small; a second protocol beside `step()` would not be.
3. **Fragmentation** -- one window with one lifetime (the mapping) beats per-record allocations.
4. **Execution time** -- contiguous in-order scans, no pointer chasing, no copies between stages.

## 5. Open questions

### Foundations

1. **Is 3.2 the idea?** The same `buffer*` is the producer's `fd_out->w` and the consumer's `fd_in->r`, and `op` is a switch instead of `read`/`write`?
2. **Where do stages live?** In-process (builtins hand over buffers directly), real pipes with `mmap`/`splice` fallback, both (fast path when every stage is a builtin), or library only for now?
3. **Fibers or state machines?** Real stacks (`ucontext` / hand-written switch) or a resumable `step()`-style function per stage?

### The window

4. **Shared window semantics.** When the producer's window fills, does the consumer see exactly `x+p .. x+n` while the producer continues in the remainder, or does the window reset once the consumer has drained it?
5. **Last stage.** Is the mmap the shared window for the whole chain, or does the last stage own a window that the earlier ones fill?
6. **Non-file sinks.** What happens when the final sink is a pipe or a tty and cannot be mapped?
7. **Growth.** `ftruncate` + `mremap` on demand, or pre-sized?
8. **Lifetime.** Is a record valid until the chain ends (arena semantics), or only until the next hand-over?

### The path side

9. **Prefix in the buffer.** Should the walker keep the current path prefix *in the output buffer* (descend appends `/name`, ascent truncates), so the buffer is both the path stack and the output?
10. **Destination type.** Should the `path_*` functions gain a `(char* buf, size_t cap)` window form returning the needed length, or should `stralloc` get a "borrowed" mode that never reallocates?
11. **In-place transforms.** May `realpath`/`readlink` rewrite bytes already in the window? May a transformer **grow** a path beyond its input, or must a stage never produce more than it consumed at any moment?
12. **Walker state.** Is the directory stack (open `DIR*`s / fds) also kept in the shared arena?

### Records

16. **Record delimiter.** NUL (safe for any filename, matches `find -print0`) or `\n`, or both selectable?
17. **Oversize record.** If one path does not fit the window: fail, grow the window, or spill like `filter_in_spill()`?
18. **Grow vs shrink.** Is the window an append-only arena (never overwrites input) or a ring that
    reclaims consumed bytes? The first wastes space; the second needs the write pointer to stay behind the read pointer.

### Scope

13. **Primary deliverable.** A demo program, real builtins (`find`, `ls -R`, `realpath`) sharing one core, or a generic `lib/` primitive?
14. **Extension of `filter_ops`.** Does this extend the existing `step()` contract or become a symmetric reserve/commit protocol beside it?
15. **Platforms.** Fibers and `mmap` differ on Windows, WASM and dietlibc; which targets must the PoC support?
