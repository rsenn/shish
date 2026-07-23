# TODO / Roadmap

Leverage-sorted list of what's still open. Fixed work lives in `git log` and
`fixes/*.patch`, not here — this file only tracks what's left to do. See
`BUGS` for confirmed, reproducible defects with repro steps.

---

## Goal 1 — POSIX conformance

`./configure` (this project's own stress test — autoconf output exercises
nested command substitution, heavy fd juggling, `eval`, here-docs,
trap/exit interplay) now runs to completion end-to-end. The `tests/posix`
conformance suite (120 files) is wired into `ctest` and runs by default.
`tests/yash` (119 more files) is wired in the same way but gated behind
its own `-DDO_YASH_TESTS=ON` (off by default — `tests/yash/random-y.tst`
hangs and only terminates via its own 120s `TIMEOUT`, `BUGS:
yash-random-y-tst-hangs`, which otherwise dominates a default `ctest`
run's wall time).

What's left is whatever the next triage pass over these suites turns up —
nothing else currently confirmed and open.

---

## Goal 2 — `src/job` cleanup

Small, isolated, low-risk items left over from a full audit of
`lib/sig`/`lib/wait`/`src/job` (most of that audit's findings are already
fixed — see `fixes/41` through `fixes/76`):

- **Delete `src/job/job_x.c`.** Byte-for-byte duplicate of
  `job_printstatus.c`, confirmed zero callers anywhere.
- **Delete `job_get`/`job_proc`/`proc_bypid`** (`src/job.h`,
  `src/job/job_get.c`) — confirmed zero callers anywhere.
- **`sh_onsig()`** (`src/sh/sh_main.c`'s SIGCHLD handler) calls
  `term_erase()`/`term_restore()`/buffered I/O directly from
  signal-handler context — none of that is async-signal-safe. Proposed
  fix worked out in `BUGS: sh-onsig-async-unsafe` (not yet implemented):
  shrink the handler to `wait_nohang()` + `job_signal()` only (both
  safe), and add a self-pipe so `term_read()`'s blocking read can
  `select()`/`poll()` for pending work instead of relying on `EINTR` —
  confirmed not viable as-is, since every signal here installs with
  `SA_RESTART` (`lib/sig/sig_action.c`), so the kernel transparently
  restarts the interrupted read without `term_read()` ever seeing it.

---

## Goal 3 — arena allocator for the AST (planned, not started)

`src/tree.h`'s AST is a graph of individually `malloc()`'d nodes
(`tree_newnode()`) plus separately `malloc()`'d string buffers hanging off
several of them — one `malloc`/`free` pair per node, even though a tree's
real lifetime is always "parse it all at once, evaluate, throw the whole
thing away" (`sh_loop.c`). `lib/arena.h` already declares the target
interface (not implemented yet):

```c
typedef struct arena_s { struct arena_block* head; size_t blocksize; } arena;

void  arena_init(arena* a, size_t blocksize);
void* arena_alloc(arena* a, size_t len);  /* bump-allocate, no per-call free */
void  arena_reset(arena* a);              /* forget contents, keep blocks for reuse */
void  arena_free(arena* a);               /* release everything back to the system */
```

Design decisions already worked out (full reasoning in git history —
2026-07-23/24 commits):

- **A stack of arenas, not one global one.** Every independent
  parse-evaluate-free scope (`sh_loop.c`, `builtin_eval.c`,
  `builtin_source.c`, `builtin_expr.c`, `prompt_parse.c`,
  `builtin_trap.c`'s inline parse) nests strictly via ordinary call-stack
  recursion — shish is single-threaded, so arenas never need to overlap
  without nesting. Push one per scope; `arena_reset()`/`arena_free()` it
  wherever `tree_free()` is called today.
- **`tree_free()` mostly disappears, not just changes signature.** Most of
  its current call sites just free a subtree still inside the current
  statement — those calls simply go away, since the dead nodes just wait
  for the enclosing arena to reset. Only the handful of true scope
  boundaries above get an `arena_reset()`/`arena_free()` call instead.
- **Two things can't live in the transient arena:** function bodies and
  trap bodies, since both must outlive the statement that defines them.
  Trap bodies already parse through their own independent `parse_init()`
  call, so they can just get their own dedicated, never-reset arena.
  Function bodies parse inline as part of the defining statement
  (`eval_function.c` already does a manual "steal the body pointer, null
  the original" trick to keep them alive past their own tree) — this
  needs either a deep-copy into long-lived storage at adoption time, or
  the parser switching allocators while inside a function body.
- **`stralloc` doesn't fit an arena** — it grows via `realloc()`, which
  can't work once other data has been bump-allocated after it. A new,
  immutable type covers the tree's own write-once-at-parse-time strings:

  ```c
  typedef struct { size_t len; char* s; } arena_str; /* NUL-terminated,
                                                         built once */
  arena_str arena_strcpy(arena* a, const char* s, size_t len);
  ```

  This replaces `nargstr.stra`, `nargparam.name`, `nfor.varn`, and
  `nfunc.name` (all populated once, during parsing, never touched again).
  `narg.stra` stays a real `stralloc` — it's populated later, at
  expansion time, not parse time. Packing a node and its string tightly
  adjacent in the arena is safe with no alignment padding, since
  `src/tree.h`'s node structs are already `__packed`.
- **Possible future: precompiled/cached AST on disk.** Serialize arena
  blocks with node pointers rewritten to offsets; on load, run one linear
  fixup pass turning offsets back into real pointers (structured like
  `tree_free()`'s own `switch(node->id)`) — after that, every existing
  tree-walking function works completely unmodified. A more invasive
  "offsets natively everywhere, zero-copy `mmap()`" design is possible but
  touches every tree-walking call site for a benefit unlikely to matter
  next to lexing/parsing cost.

---

## Also open

- **Line-editing/terminal-abstraction/key-bindings rewrite** — a
  design-sized project inherited from the old `TODO` file, not a fixable
  bug. Minimal filename tab-completion (`src/term/term_complete.c`) is the
  only piece of this done so far.
