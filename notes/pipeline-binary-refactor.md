# Refactor: `struct npipe` from n-ary list to binary operator

## Goal

`struct npipe` (`N_PIPELINE`, 3.9.2) is currently an n-ary node: `ncmd`
count + a `cmds` list threaded through `->next`. Every other list-like
construct in the language (`&&`, `||`) is instead a binary operator
(`struct nandor`, `left`/`right`). Reshape `npipe` the same way:

```c
struct npipe {
  enum kind id;
  unsigned bgnd : 1;
  union node* next;
  union node* left;  /* pipeline so far (single cmd, or nested N_PIPELINE) */
  union node* right;  /* the command being piped into */
} SHISH_TREE_PACKED;
```

`a | b | c` parses left-associative, same shape as `a && b && c`:

```
N_PIPELINE
├─ left:  N_PIPELINE
│         ├─ left:  a
│         └─ right: b
└─ right: c
```

Consequence worth relying on throughout: `right` is *always* a single
command, never itself `N_PIPELINE`. `left` is either a single command
(innermost stage) or a nested `N_PIPELINE`. So the tree is a left
spine — walking `->npipe.left` while `id == N_PIPELINE` and taking
`->npipe.right` at each step reconstructs the original left-to-right
command order, same order `->cmds`/`->next` gives today.

## Why now

- `parse_pipeline()` hand-rolls list bookkeeping (`cmdptr`, `tree_link`,
  manual `ncmd++`) that every other binary construct in `parse.c`
  doesn't need.
- Every generic tree walker (`tree_free`, `tree_copy`, `tree_cat`,
  `debug_node`) special-cases `N_PIPELINE` as a sublist instead of
  reusing the `left`/`right` recursion they already have for
  `N_AND`/`N_OR`.
- `eval_pipeline()`'s core loop is fine and mostly doesn't need to
  change — the payoff is entirely on the parse/tree side.

## The landmine: `tree_location.c`'s field aliasing — FIXED

`tree_location.c:55-65` used to read `node->npipe.cmds` for **five**
node kinds, not just `N_PIPELINE`:

```c
switch(node->id) {
  case N_PIPELINE:
  case N_BRACEGROUP:
  case N_SUBSHELL:
  case N_FOR:
  case N_CASENODE:
    if(tree_location(node->npipe.cmds, loc))
      return 1;
    break;
  ...
```

`N_BRACEGROUP`/`N_SUBSHELL` are really `struct ngrp` (`.cmds` after
`id, bgnd:1, next, rdir`), `N_FOR` is `struct nfor` (`.cmds` after
`id, bgnd:1, has_in:1, next, rdir`), `N_CASENODE` is `struct ncasenode`
(`.cmds` after `id, dummy:1, next, pats`). This only compiled/worked
because those structs' `cmds` pointer happened to land at the same
byte offset as `npipe.cmds` (bitfields + padding lining up by
construction, not by any named guarantee) — the code was exploiting
union aliasing on purpose to avoid a switch arm per kind.

Removing `npipe.cmds` would have broken this for all five cases at
once, silently (wrong pointer read, not a compile error, since the
other four kinds still have a `cmds` field under their own name). This
has already been split apart, ahead of the rest of the refactor, so
the struct change below isn't racing against a hidden dependency:

```c
switch(node->id) {
  case N_PIPELINE:
    if(tree_location(node->npipe.cmds, loc))
      return 1;
    break;
  case N_BRACEGROUP:
  case N_SUBSHELL:
    if(tree_location(node->ngrp.cmds, loc))
      return 1;
    break;
  case N_FOR:
    if(tree_location(node->nfor.cmds, loc))
      return 1;
    break;
  case N_CASENODE:
    if(tree_location(node->ncasenode.cmds, loc))
      return 1;
    break;
  default: break;
}
```

Behavior-preserving — confirmed with `offsetof`, not just assumed:
`npipe.cmds`, `ngrp.cmds`, `nfor.cmds`, `ncasenode.cmds` all land at
byte offset 24 in the pre-fix layout, so the split reads exactly the
same memory as before. Now each kind reads its own named field
instead of riding the `npipe.cmds` coincidence, so `N_PIPELINE` can
freely lose `.cmds` later without touching the other four cases again.

Full `ctest` run done both with and without this change: same 5
`posix/*-p.tst` failures (`simple-p`, `tilde-p`, `trap-p`, `umask-p`,
`unset-p`) plus the known-flaky `sig*-p.tst` timing failures
(`signal-tests-vary-with-machine-load` in `BUGS`) on both, confirming
this change introduces no regressions.

Checked every other `npipe.cmds`/`npipe.ncmd` site in the tree
(`tree_free.c`, `tree_copy.c`, `tree_cat.c`, `debug_node.c`,
`eval_function.c`) — all of those switch arms are `case N_PIPELINE:`
alone, no aliasing with other kinds. `tree_location.c` was the only
landmine, and it's now defused.

## `eval_pipeline()`: keep the fork loop, add a flatten step

`eval_pipeline()`'s loop (`eval/eval_pipeline.c:46-150`) walks
`npipe->cmds` via `->next`, forking `ncmd - 1` pipe-connected children
plus the last stage, all as procs of one `job_new(npipe->ncmd)`. That
loop, the `prevfd` chaining, the `job_fork`/`fdstack_npipes` handling —
none of it needs to change in shape. What it needs is:

1. A **count** of pipeline stages, upfront, to size `job_new(n)` (job's
   proc array is fixed-size, allocated once, no realloc path exists —
   see `job_new.c:14`).
2. The stages **in left-to-right order** to preserve current fd-wiring
   behavior (`in`/`out` per stage, `prevfd` chained forward).

Recommendation: **flatten at the top of `eval_pipeline()`**, once,
into a local array of `union node*`, then run the existing loop
against that array instead of `->next`. Concretely:

```c
unsigned n = count(npipe);          /* walk ->npipe.left spine */
union node** stage = alloca(n * sizeof *stage);
/* fill right-to-left while unwinding the spine, or left-to-right
   with a second pass — either way, O(n), and n is already bounded
   by job_new()'s own allocation */
...
job = job_new(n);
for(i = 0; i < n; i++) {
  node = stage[i];
  /* rest of the existing loop body, keyed off i == n-1 instead of
     node->next for "is this the last stage" */
}
```

This is deliberately *not* a recursive eval (no `eval_pipeline_rec`
forking left before right) — recursion would work too (left's own
last stage's pipe write-end feeds right's stdin, execution order left
lands first which matches wiring), but it reintroduces the same
"count everything before you can allocate the job" problem one level
down, for no benefit: `pipes`/`in`/`out`/`prevfd` and the job's flat
proc array are all inherently a flat, ordered structure regardless of
how the AST nests. Flatten once at the boundary, keep the rest of
`eval_pipeline()` untouched.

Open question to confirm with you before implementing: **alloca vs.
`job_new`-style `alloc`/`alloc_free`** for the stage array. The file
already conditionally uses `alloca` (`HAVE_ALLOCA`) for per-stage `fd`
structs, so `alloca` fits the existing style, but pipeline depth is
user-controlled (`a | b | c | ...`) — worth deciding whether that's
an acceptable stack-growth vector or whether it should go through
`alloc`/`alloc_free` like `pipes` does a few lines below.

## Parser: `parse_pipeline()` gets simpler

Current version pre-allocates the first node, then threads a `cmdptr`
through a `do/while`, incrementing `npipe->ncmd` by hand. Binary-operator
version collapses to the same shape as how `&&`/`||` chains presumably
parse (check `parse_andor.c` or equivalent for the exact idiom this
should match):

```c
node = parse_command(p, P_DEFAULT);
if(node == NULL) return NULL;

while((tok = parse_gettok(p, P_DEFAULT)) == T_PIPE) {
  union node *pipeline, *right;

  right = parse_command(p, P_SKIPNL);
  if(right == NULL) {
    parse_error(p, T_NAME | T_WORD);
    tree_free(node);
    return NULL;
  }

  pipeline = tree_newnode(N_PIPELINE);
  pipeline->npipe.bgnd = 0;
  pipeline->npipe.left = node;
  pipeline->npipe.right = right;
  node = pipeline;
}
p->pushback++;
```

Note the error path changes: today a failed `parse_command` after `|`
frees the whole `pipeline` node (which owns the flat `cmds` list built
so far). In the binary form, `node` on entry to the loop body already
*is* the whole left-associated tree built so far, so `tree_free(node)`
on failure is the equivalent free — just double check no leak on the
first iteration (bare `a |` with nothing after) vs. later ones (`a | b
|` with nothing after) — same call, but worth a `fixed.sh`/`parse-
error.sh`-style test for both since the old code path handled them via
different variables (`pipeline` vs. implicitly whatever `node` held).

## Files to touch, in order

1. `src/tree.h` — `struct npipe`: drop `ncmd`, rename `cmds` → `left`
   + add `right`.
2. `src/parse/parse_pipeline.c` — rewrite per above.
3. `src/tree/tree_free.c`, `src/tree/tree_copy.c`, `src/tree/tree_cat.c`,
   `src/debug/debug_node.c` — mirror each file's existing `N_AND`/
   `N_OR` case for `N_PIPELINE` (recurse `left` then `right`; `tree_cat`
   additionally needs to keep emitting `" | "` between stages, which
   for a left-recursive binary tree means printing `left` normally and
   *not* wrapping it in parens — check whether `tree_cat`'s `N_AND`/
   `N_OR` case already parenthesizes nested chains or not, so pipeline
   output formatting doesn't change).
4. ~~`src/tree/tree_location.c` — split `N_PIPELINE` out of the
   aliased switch (see landmine section); leave the other four kinds
   reading their own field names.~~ **Done**, landed ahead of the rest
   of this refactor.
5. `src/eval/eval_function.c` (`hash_commands_in_node`) — `N_PIPELINE`
   case: recurse `node->npipe.left` + `node->npipe.right` like the
   `N_AND`/`N_OR` case already does, instead of walking a `->next`
   chain. Also check the trailing "walk the `next` pointer for lists"
   guard (`if(node->id != N_PIPELINE && node->id != N_CASE)`) — still
   correct as-is, since a pipeline's own `->next` is still "what
   follows this whole pipeline in its enclosing list," unrelated to
   `left`/`right`.
6. `src/eval/eval_pipeline.c` — add the flatten step, key the loop off
   an index/array instead of `->next`; `npipe->ncmd` no longer exists,
   so `job_new()` takes the freshly counted `n`.
7. `src/tree/tree_nodesizes.c` — no change needed, `sizeof(struct
   npipe)` picks up the new layout automatically.

`src/fdstack/fdstack_npipes.c` and `src/fdstack/fdstack_pipe.c` are a
naming coincidence (`npipes` = count of command-substitution pipes to
wire, unrelated to `struct npipe`) — confirmed, not in scope.

## Testing

No dedicated `tests/pipeline.sh` exists today — pipeline behavior is
only incidentally exercised through other builtins' tests (`builtin-
tee.sh`, `builtin-wc.sh`, `here-doc.sh`, `fixed.sh`). Given this
refactor touches parse, all four generic tree walkers, and eval's job
sizing, add `tests/pipeline.sh` covering, before starting the refactor
(so it can validate before/after):

- 2-stage and 4+-stage pipelines, exit status of the last stage,
  `$?` immediately after (per the comment in `eval_pipeline.c:170-178`
  about `sh->exitcode` syncing).
- `a | b &` (backgrounded pipeline) exit status handling.
- a pipeline as the RHS/LHS of `&&`/`||`, inside `$(...)`, inside a
  function body (exercises `hash_commands_in_node` and `tree_copy`).
- `shformat` round-trip on a multi-stage pipeline (exercises
  `tree_cat`).
- the two parse-error shapes noted above (`a |`, `a | b |`).

Run the full suite (`ctest`) after each file in the "files to touch"
list where that's meaningful, not just once at the end — steps 3-5 are
independent of each other and of the parser rewrite, so bugs are
easier to localize if verified incrementally.

## Open decisions for you

1. `alloca` vs `alloc`/`alloc_free` for the eval-time stage array
   (stack growth vs. heap, see above).
2. Confirm left-associative shape (`right` always a leaf) is the
   intended direction — this doc assumes it because it's what the
   current parser already produces and what keeps `eval_pipeline`'s
   flatten step a simple linear walk; a right-associative shape would
   also work but inverts which side is guaranteed to be a leaf.
3. Whether `tests/pipeline.sh` should land as a separate prerequisite
   step before the refactor (recommended) or alongside it.

## Open idea (not in scope for this refactor): cooperative builtin-to-builtin pipes

Raised alongside this refactor, worth recording but deliberately kept
separate — it's a scheduling change to `eval_pipeline()`, independent
of whether `npipe` is n-ary or binary, and shouldn't block the binary-
operator work above.

**The idea.** Today, every pipeline stage forks (`job_fork()`,
`eval_pipeline.c:112`), even when a stage is a builtin — the comment
at `eval_pipeline.c:114-116` notes job control is explicitly skipped
for in-pipe commands, but the fork itself still happens. When *both*
sides of one pipe stage are builtins, there's no real subprocess work
happening — a `fork()` (plus its `execve()`-less overhead: address
space duplication, an extra job slot, extra `wait()` bookkeeping) is
paid just to shuttle bytes between two pieces of code that are both
already linked into this process. The proposal: run such a pair
cooperatively, in-process, switching control directly between the
writer and the reader instead of going through a real pipe fd and two
kernel processes.

**Why `setjmp`/`longjmp` alone doesn't get there.** They save/restore
the instruction pointer, stack pointer, and callee-saved registers,
but not a copy of the stack contents — resuming a `jmp_buf` only works
while the frame it was captured in is still the live, unmodified top
of *that same stack*. That holds for the common "abort a deep call
chain back to an outer handler" use (`sh`'s own error/trap handling,
elsewhere in this codebase, presumably works this way) because control
only ever travels outward to an ancestor frame that's still on the
stack.

A producer/consumer pair is different: the writer needs to pause
*mid-loop*, hand control to the reader, and later come back to the
*exact same point*, deeper in its own call chain, with its own locals
intact — while the reader has been running and pushing its own frames
on that same shared stack in the meantime. A `longjmp` back into the
writer at that point is jumping into a frame that (from the C
implementation's perspective) may already have been overwritten by
whatever the reader called in between. This isn't a portability
detail, it's `setjmp`/`longjmp`'s documented contract (C99 7.13.2.1):
undefined behavior if the function containing the corresponding
`setjmp` has returned — and "returned" is a good proxy for "some other
code reused this stack region since," which is exactly what happens
here.

**What actually works: separate stacks per builtin.** Real two-sided
suspend/resume needs each side on its *own* stack, so switching
between them is a stack-pointer swap, not a jump within one shared
stack. `ucontext.h` (`getcontext`/`makecontext`/`swapcontext`) is the
POSIX-standard way to get that; a hand-rolled fiber (allocate a stack,
prime it, swap `sp`/`pc` by hand per architecture) is the fallback
where `ucontext.h` isn't available.

That availability gap is the real blocker for *this* project
specifically, not a general portability nitpick: `cfg-cmake.sh`
targets MinGW (`cfg-mingw32`/`cfg-mingw64`) and MSYS
(`cfg-msys`/`i686-pc-msys`/`x86_64-pc-msys`, both present in this
repo's own `build/`), where `ucontext.h` doesn't exist at all (Windows
has no equivalent in its native API without pulling in fiber APIs
through a compatibility shim), plus Emscripten/WASM
(`cfg-emscripten`/`cfg-wasm`) and Android/Termux, where support is
inconsistent or absent. A cooperative scheduler here would need either
a real per-platform fiber implementation (a second implementation of
`fdstack`-style state per builtin call, essentially) or to silently
fall back to the current fork-per-stage behavior on those targets —
either way it's a substantially bigger, separately-scoped project than
this pipeline binary-operator refactor, with its own portability
matrix to validate across every `cfg-*` target in `cfg-cmake.sh`.

Emscripten/WASM (`cfg-emscripten`/`cfg-wasm`) does have its own
fiber primitive — `emscripten/fiber.h`'s `emscripten_fiber_init()` /
`emscripten_fiber_swap()`, a real coroutine with a separate C stack
per fiber, confirmed against the current header and docs. It doesn't
close the portability gap above, though, it just adds a third
implementation to it:

- It requires linking with `-sASYNCIFY`, which instruments functions
  that might unwind and carries real code-size and runtime cost —
  not a free addition to the wasm build.
- Each fiber needs *two* separate memory regions sized and allocated
  by hand: a C stack (16-byte aligned, grows down) and an Asyncify
  stack (grows up, holds the call-stack/locals snapshot Asyncify
  needs to unwind/rewind).
- It's Emscripten-specific API surface — MinGW, MSYS, and
  Android/Termux still need `ucontext.h` (where present) or a
  hand-rolled fallback, so this covers exactly one of the `cfg-*`
  targets that lacks `ucontext.h`, not all of them.

So the realistic shape of "separate stacks per builtin," if this is
ever pursued, is three code paths behind whatever abstraction wraps
them: `ucontext.h` on the targets that have it, `emscripten/fiber.h`
(plus `-sASYNCIFY`) for `cfg-emscripten`/`cfg-wasm`, and a hand-rolled
stack-switch for MinGW/MSYS/anything else — each independently
verified across `cfg-cmake.sh`'s target list, not a single portable
implementation.

**Narrower alternative, if this is worth pursuing later.** Restrict it
to the single case where it's cheap to reason about: a lone
builtin-writes-then-exits feeding a lone builtin-reads-then-exits, with
no other pipeline stage on either side, and neither builtin needing to
interleave reads and writes with anything else. That degenerates to an
ordinary nested function call (writer calls reader directly when its
output buffer is ready, no scheduler, no saved continuation, no
`ucontext` needed) rather than true coroutines — much less machinery,
but only covers the simplest pipeline shape; anything with 3+ stages
or a builtin on both a read and a write side at once still needs the
general mechanism above.
