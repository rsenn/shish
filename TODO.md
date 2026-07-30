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

A separate, much larger real-world stress test — gettext-tools'
`configure` (autoconf-generated, gnulib-heavy, ~30k lines) — now also
runs to completion (~2.5 min, no hang/crash/core dump) as of 2026-07-30,
after fixing three bugs found chasing a reported hang in `sh_forked.c`'s
`for(sh = sh->parent; sh; sh = next)` loop: `eval_exit()` ("exit" inside
a function/subshell) had the same commented-out fdstack/varstack/source
unwind bug already fixed for `eval_return`/`eval_jump` (`fixes/97`,
`fixes/99`), plus its own extra wrinkle — it deliberately walks past any
number of `E_FUNCTION` frames to reach the nearest subshell/root, so it
also had to pop each skipped function call's `sh_push()`ed `struct env`,
or that env dangles and corrupts `sh->parent` the moment its stack slot
is reused (`fixes/101`). A second, unrelated bug in the same fix: a bogus
`if(e == sh->parent->eval) return` early-out silently turned `exit` into
a no-op whenever it was called two or more function calls deep in the
same process. Chasing the crash further surfaced a third, independent
bug in `eval_function()`, which stole (moved, then nulled) a function
definition's name/body pointers out of its AST node — safe only if that
node is evaluated once, so it segfaulted the moment the same node was
evaluated again (a function defined inside a loop, or inside one of
shish's in-process `(...)` subshells); fixed via a new `tree_copy()`
helper (`fixes/102`). The script now stops instead on a real
`configure`-level detection failure (`socklen_t`), which traces back to
`BUGS: confdefs-h-duplication`, still open.

What's left is whatever the next triage pass over these suites turns up,
plus `BUGS: yash-random-y-tst-hangs`, found and narrowed (not yet fixed)
while doing exactly that on 2026-07-29 — now points at the file's own
busy-`until` `$RANDOM`-comparison loops rather than the later
subshell-piping cases originally suspected. A related-looking
`redir-p.tst` hang found the same day turned out to be a real,
deterministic bug once properly isolated (an 8-deep `<&` dup chain off
a freshly-opened fd resolved before its source ever got a chance to
open) — fixed as `fixes/89` (fd-resolution ordering, both the builtin
and forked-external-command paths) plus `fixes/88` (`builtin_cat()`
spinning instead of erroring on the resulting bad fd). What first
looked like Release-vs-MinSizeRel build sensitivity for that one turned
out to be two stacked testing mistakes instead (see `BUGS`) — a caution
for `yash-random-y-tst-hangs`'s own still-unconfirmed build-sensitivity
claim, not evidence either way.

---

## Goal 2 — `src/job` cleanup

Small, isolated, low-risk items left over from a full audit of
`lib/sig`/`lib/wait`/`src/job` (most of that audit's findings are already
fixed — see `fixes/41` through `fixes/76`):

- **Delete `src/job/job_x.c`.** Byte-for-byte duplicate of
  `job_printstatus.c`, confirmed zero callers anywhere.
- **Delete `job_get`/`job_proc`/`proc_bypid`** (`src/job.h`,
  `src/job/job_get.c`) — confirmed zero callers anywhere.

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
  Function bodies parse inline as part of the defining statement;
  `eval_function.c` used to keep them alive past their own tree with a
  manual "steal the body pointer, null the original" trick, but that
  broke (segfault) the moment the same definition node was evaluated
  more than once — a function defined inside a loop, or inside one of
  shish's in-process `(...)` subshells — since the second visit found
  the pointers already nulled from the first (fixed 2026-07-30,
  `fixes/102`). It now does the "deep-copy into long-lived storage at
  adoption time" option this bullet already anticipated, via a new
  generic `tree_copy()` (`src/tree/tree_copy.c`) that mirrors
  `tree_free()`'s per-kind switch. Once the arena lands, `tree_copy()`
  is exactly the function that needs to switch from allocating loose
  nodes to bump-allocating into a function's own dedicated arena instead
  — the "parser switching allocators while inside a function body"
  alternative is no longer needed now that a working copy path exists.
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
