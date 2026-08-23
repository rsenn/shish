# Cooperative pipeline stages via `task.h`

Follow-on to [`pipeline-binary-refactor.md`](pipeline-binary-refactor.md)'s
"Open idea" section (cooperative builtin-to-builtin pipes instead of
`fork()` per stage). That section identified the missing piece as "a
real fiber, not `setjmp`/`longjmp`" and named `ucontext.h` /
`emscripten/fiber.h` / a hand-rolled fallback as the three code paths
it would take. `task.h` (`experiments/task-pipeline/`) is that engine,
built and validated against two of those three backends -- libaco
(native) and `emscripten/fiber.h` (wasm) -- with two working demos,
`test-task.c` and `test-task-buffers.c`. This document is about the
next question: **could shish itself run pipeline stages this way,
instead of `fork()`ing each one?**

## The question

> Could we run multiple shell evaluation contexts (sub-commands of a
> pipeline) concurrently using `task.h` and stream output from each
> stage into the next?

Mechanically, yes -- it composes with what's already validated. Each
pipeline stage's `eval_tree()` call would run on its own `task_t`
instead of forking a process, and a read/write on the virtual pipe
between two *builtins* (external commands still have to `fork()`/
`execve()`, they're not shish's own code) would become
`task_yield()`/`task_resume()` instead of a real `read()`/`write()`
syscall -- routed through a fiber-backed variant of `lib/buffer.c`'s
`op`/`fd` mechanism, sitting alongside the real one.

Two things are worth being clear-eyed about before this goes further:

1. **It is not parallelism.** `task.h`'s coroutines are cooperative
   and single-threaded -- only one stage ever actually runs at a
   time, handing off control explicitly at each `task_yield()`. The
   win is avoiding `fork()`'s cost (address-space duplication, a job
   slot, `wait()` bookkeeping), not running pipeline stages on
   separate cores. "Concurrently" here has to mean *interleaved*, not
   *simultaneous*.

2. **`fork()` currently gives shish's pipeline stages isolation for
   free.** Each real pipeline stage today gets its own copy-on-write
   address space -- its own view of shell variables, `$$`, cwd,
   traps, `$?`. Coroutines sharing one process would share all of
   that directly instead, so making this correct means explicitly
   saving/restoring shell state at every yield/resume boundary. That
   is arguably a bigger job than the I/O plumbing itself, and it's
   the open question this document doesn't answer yet -- see
   "Next steps" below.

## What's validated so far

`task.h` is a header-only, common-denominator coroutine API over
libaco and Emscripten's Fiber API (backend picked automatically via
`__EMSCRIPTEN__`, or forced with `TASK_USE_ACO`/`TASK_USE_EMSCRIPTEN`):

```c
task_t* task_create(task_fn fn, size_t stack_size);
void    task_destroy(task_t* t);
void*   task_resume(task_t* t, void* arg);
void*   task_yield(task_t* self, void* arg);
int     task_done(const task_t* t);
```

A single `void*` mailbox changes hands at every transfer, in both
directions -- the same shared-`arg` mailbox pattern libaco's own
tutorials use. `arg` may point into the caller's own stack frame: if
that frame belongs to a suspended task, the pointer stays valid across
the hand-off because a task's stack is never reused while it's merely
suspended -- unlike `setjmp()`/`longjmp()`, which share one stack and
can't preserve a suspended frame's locals once another call has run on
top of it. That distinction is the entire reason real fibers are
needed here, not just a cheaper `setjmp()`.

One topology constraint, inherited from libaco and deliberately kept
on the Emscripten backend too for portability: a task's yield always
returns control to whoever most recently resumed *that task*, not to
an arbitrary peer. A chain has to be driven by a single caller (e.g.
`main()`, or in shish's case presumably `eval_pipeline()`) that
resumes stage 0, feeds its output into stage 1, and so on -- tasks
don't resume each other directly.

Both demos below were built and run against **both** backends
(libaco natively, Emscripten via `node` under `-sASYNCIFY`), and the
libaco/native builds were additionally run clean under `valgrind`.
Output matched byte-for-byte between backends in both cases.

All three source files live in `experiments/task-pipeline/`, alongside
`build-test-task.sh`; the relative paths quoted in their build
comments below (`../../../libaco`, `../../lib`, `../../build/...`) are
from that directory.

## `task.h`

```c
/* task.h -- minimal stackful-coroutine API, common denominator of
 * libaco (../../../libaco/aco.h) and Emscripten's built-in Fiber API
 * (emscripten/fiber.h).
 *
 * Backend is chosen automatically: emscripten/fiber.h under
 * __EMSCRIPTEN__, libaco everywhere else. Force one explicitly by
 * defining TASK_USE_ACO or TASK_USE_EMSCRIPTEN before including this
 * file.
 *
 * Header-only: every function is `static`, so including this file in
 * more than one translation unit is safe (no separate task.c to link).
 *
 *   typedef void (*task_fn)(task_t* self, void* arg);
 *
 *   task_t* task_create(task_fn fn, size_t stack_size);
 *   void    task_destroy(task_t* t);
 *   void*   task_resume(task_t* t, void* arg);
 *   void*   task_yield(task_t* self, void* arg);
 *   int     task_done(const task_t* t);
 *
 * Data model: a single void* "mailbox" changes hands at every
 * transfer, in both directions --
 *
 *   task_resume(t, arg)  hands `arg` to t, switches into it, and
 *                        returns whatever t next hands back via
 *                        task_yield() (or its last task_yield()'s
 *                        value again, if t has already finished).
 *
 *   task_yield(self, arg) hands `arg` back to whoever called
 *                        task_resume() on self, suspends self, and
 *                        returns (once self is resumed again)
 *                        whatever the next task_resume() call passed.
 *
 * `arg` may point into the caller's own stack frame -- if that
 * frame belongs to a *task* rather than to the top-level caller, the
 * pointer stays valid across the hand-off because a task's stack is
 * never reused by anything else while the task is merely suspended
 * (unlike setjmp()/longjmp(), which share one stack and cannot
 * preserve a suspended frame's locals once another call has run on
 * top of it -- see ../../notes/pipeline-binary-refactor.md's "Open
 * idea" section for why that distinction is the whole point of using
 * real fibers).
 *
 * Topology constraint inherited from libaco: a task's yield always
 * returns control to whoever most recently resumed *that task*, not
 * to an arbitrary peer. Chain tasks by having a single driver (e.g.
 * main()) resume stage 0, feed its output into stage 1, and so on --
 * don't have one task directly resume another. The Emscripten backend
 * could support true peer-to-peer resumes, but this header keeps both
 * backends to the same contract so code stays portable between them.
 *
 * task_create()'s entry function must never simply return -- both
 * backends require the coroutine to hand control away explicitly.
 * This header enforces that uniformly: once `fn` returns, the task is
 * marked done and parked in an infinite yield loop instead of falling
 * off the end (undefined for libaco's aco_exit()-less return, and
 * explicitly documented as ending the whole program for Emscripten
 * fibers). Call task_destroy() once a task is task_done() and won't
 * be resumed again.
 */
#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <stdlib.h>

#if !defined(TASK_USE_ACO) && !defined(TASK_USE_EMSCRIPTEN)
#if defined(__EMSCRIPTEN__)
#define TASK_USE_EMSCRIPTEN 1
#else
#define TASK_USE_ACO 1
#endif
#endif

typedef struct task task_t;
typedef void (*task_fn)(task_t* self, void* arg);

#if defined(TASK_USE_ACO)

#define TASK_BACKEND_NAME "libaco"

#include "../../../libaco/aco.h"

struct task {
  task_fn fn;
  void* xfer;
  int done;
  aco_t* co;
  aco_share_stack_t* sstk;
};

/* one hub coroutine per thread, lazily created -- every task_t's
 * aco_t is created with this as its (fixed) main_co, since libaco
 * always yields back to the co's creation-time main_co */
static __thread aco_t* task__aco_main = NULL;

static void
task__aco_ensure_main(void) {
  if(task__aco_main == NULL) {
    aco_thread_init(NULL);
    task__aco_main = aco_create(NULL, NULL, 0, NULL, NULL);
  }
}

static void
task__aco_trampoline(void) {
  task_t* self = (task_t*)aco_get_arg();

  self->fn(self, self->xfer);
  self->done = 1;

  for(;;)
    aco_yield();
}

static task_t*
task_create(task_fn fn, size_t stack_size) {
  task_t* t;

  task__aco_ensure_main();

  t = (task_t*)calloc(1, sizeof *t);
  if(!t)
    return NULL;

  t->fn = fn;
  t->sstk = aco_share_stack_new(stack_size);
  t->co = aco_create(task__aco_main, t->sstk, 0, task__aco_trampoline, t);

  return t;
}

static void
task_destroy(task_t* t) {
  aco_destroy(t->co);
  aco_share_stack_destroy(t->sstk);
  free(t);
}

static void*
task_resume(task_t* t, void* arg) {
  t->xfer = arg;
  aco_resume(t->co);
  return t->xfer;
}

static void*
task_yield(task_t* self, void* arg) {
  self->xfer = arg;
  aco_yield1(self->co);
  return self->xfer;
}

static int
task_done(const task_t* t) {
  return t->done;
}

#elif defined(TASK_USE_EMSCRIPTEN)

#define TASK_BACKEND_NAME "emscripten fiber"

#include <emscripten/fiber.h>

#ifndef TASK_DEFAULT_C_STACK_SIZE
#define TASK_DEFAULT_C_STACK_SIZE (64 * 1024)
#endif
#ifndef TASK_ASYNCIFY_STACK_SIZE
#define TASK_ASYNCIFY_STACK_SIZE (4 * 1024)
#endif

struct task {
  task_fn fn;
  void* xfer;
  int done;
  emscripten_fiber_t fiber;
  emscripten_fiber_t* resumer;
  void* c_stack;
  void* asyncify_stack;
};

/* the thread's original (non-fiber) execution context, captured once
 * so task_resume() has a valid "from" fiber to swap out of even when
 * called directly from main() rather than from another task */
static __thread emscripten_fiber_t task__em_root;
static __thread void* task__em_root_asyncify_stack = NULL;
static __thread emscripten_fiber_t* task__em_current = NULL;

static void
task__em_ensure_root(void) {
  if(task__em_root_asyncify_stack == NULL) {
    task__em_root_asyncify_stack = malloc(TASK_ASYNCIFY_STACK_SIZE);
    emscripten_fiber_init_from_current_context(
        &task__em_root, task__em_root_asyncify_stack, TASK_ASYNCIFY_STACK_SIZE);
    task__em_current = &task__em_root;
  }
}

static void
task__em_trampoline(void* arg) {
  task_t* self = (task_t*)arg;

  self->fn(self, self->xfer);
  self->done = 1;

  for(;;)
    emscripten_fiber_swap(&self->fiber, self->resumer);
}

static task_t*
task_create(task_fn fn, size_t stack_size) {
  task_t* t;

  task__em_ensure_root();

  if(stack_size == 0)
    stack_size = TASK_DEFAULT_C_STACK_SIZE;

  t = (task_t*)calloc(1, sizeof *t);
  if(!t)
    return NULL;

  t->fn = fn;
  t->c_stack = malloc(stack_size);
  t->asyncify_stack = malloc(TASK_ASYNCIFY_STACK_SIZE);

  emscripten_fiber_init(&t->fiber, task__em_trampoline, t, t->c_stack,
                         stack_size, t->asyncify_stack, TASK_ASYNCIFY_STACK_SIZE);

  return t;
}

static void
task_destroy(task_t* t) {
  free(t->c_stack);
  free(t->asyncify_stack);
  free(t);
}

static void*
task_resume(task_t* t, void* arg) {
  emscripten_fiber_t* from = task__em_current;

  t->xfer = arg;
  t->resumer = from;
  task__em_current = &t->fiber;

  emscripten_fiber_swap(from, &t->fiber);

  task__em_current = from;
  return t->xfer;
}

static void*
task_yield(task_t* self, void* arg) {
  self->xfer = arg;
  emscripten_fiber_swap(&self->fiber, self->resumer);
  return self->xfer;
}

static int
task_done(const task_t* t) {
  return t->done;
}

#else
#error "task.h: no backend selected"
#endif

#endif /* TASK_H */
```

## `test-task.c`

A 4-stage chain, 5 items streamed through it. Each stage keeps its own
running counter across repeated resumes (proving real suspend/resume,
not one-shot calls), and hands its output downstream as a pointer into
its own suspended stack frame.

```c
/* test-task.c -- demo for task.h: a chain of coroutine stages, each
 * one transforming a value and handing it to the next, driven by a
 * stream of items rather than a single one-shot call.
 *
 * Each stage keeps running per-stage state (a counter) across
 * repeated task_resume() calls, and the pointer handed downstream
 * points into the *sending* stage's own suspended stack frame -- it
 * survives the hand-off because that stage's stack is parked, not
 * torn down, until it's resumed again. That's the property this demo
 * exists to prove out (see task.h's header comment and
 * ../../notes/pipeline-binary-refactor.md's "Open idea" section).
 *
 * Build (libaco backend):
 *   cc -O2 -Wall -o test-task test-task.c \
 *     ../../../libaco/aco.c ../../../libaco/acosw.S -I../../../libaco
 *
 * Build (Emscripten backend):
 *   emcc -O2 -sASYNCIFY -o test-task.html test-task.c
 */
#include "task.h"

#include <stdio.h>

#define N_STAGES 4
#define N_ITEMS 5

static void
stage_fn(task_t* self, void* arg) {
  int total = 0;
  void* in = arg;
  int out;

  for(;;) {
    if(in == NULL)
      return; /* end-of-stream sentinel: finish, no further yield needed --
                 task.h parks a finished task and reports self->xfer (still
                 NULL from this resume) back to the caller on its own */

    out = *(int*)in * 2 + total;
    total++;

    in = task_yield(self, &out);
  }
}

int
main(void) {
  task_t* stage[N_STAGES];
  int items[N_ITEMS] = {1, 2, 3, 4, 5};
  int results[N_ITEMS];
  int i, s;

  printf("task.h backend: %s\n", TASK_BACKEND_NAME);

  for(s = 0; s < N_STAGES; s++)
    stage[s] = task_create(stage_fn, 0);

  for(i = 0; i < N_ITEMS; i++) {
    void* val = &items[i];

    for(s = 0; s < N_STAGES; s++)
      val = task_resume(stage[s], val);

    results[i] = *(int*)val;
  }

  /* drain: tell every stage the stream is over so it exits its loop */
  for(s = 0; s < N_STAGES; s++)
    task_resume(stage[s], NULL);

  for(i = 0; i < N_ITEMS; i++)
    printf("items[%d] = %d -> chained through %d stages -> %d\n", i,
           items[i], N_STAGES, results[i]);

  for(s = 0; s < N_STAGES; s++) {
    if(!task_done(stage[s])) {
      fprintf(stderr, "stage %d did not finish cleanly\n", s);
      return 1;
    }
    task_destroy(stage[s]);
  }

  return 0;
}
```

Output (identical on both backends):

```
task.h backend: libaco
items[0] = 1 -> chained through 4 stages -> 16
items[1] = 2 -> chained through 4 stages -> 47
items[2] = 3 -> chained through 4 stages -> 78
items[3] = 4 -> chained through 4 stages -> 109
items[4] = 5 -> chained through 4 stages -> 140
```

## `test-task-buffers.c`

Same shape, but each stage's payload is a libowfat `buffer`
(`lib/buffer.h`) instead of a plain `int` -- closer to what a real
shish pipeline stage would actually pass around. Every stage owns a
scratch `buffer` set up with `BUFFER_INIT_FREE(buffer_op_write, -1,
space, sizeof space)`: `fd = -1` so that if anything ever mistakenly
triggered a real flush, it fails loudly (`EBADF`) instead of touching
an unrelated fd. `buffer_frombuf()` bridges each hop, because
`buffer.p` means "write cursor" in write mode but "read cursor" in
read mode -- a buffer that was just written to can't be handed
straight to `buffer_getc()`.

```c
/* test-task-buffers.c -- like test-task.c, but each stage's payload is
 * a libowfat `buffer` (lib/buffer.h) instead of a plain int: every
 * stage owns a scratch buffer allocated with BUFFER_INIT_FREE() and
 * fd = -1 (never a real file descriptor -- if anything ever tried to
 * flush/refill it through the real op/fd path, buffer_op_write(-1,...)
 * just fails loudly with EBADF instead of touching some unrelated fd).
 *
 * Each task reads from the buffer the *previous* task wrote to, and
 * writes into its own output buffer for the *next* task to read.
 * `buffer_frombuf()` (lib/buffer.h) is the glue at each hop: it wraps
 * a finished, already-written byte range as a fresh read-only buffer
 * (dummy read op, no fd touched at all) -- that's necessary because
 * `buffer`'s `.p` field means "bytes read so far" in read mode but
 * "bytes written so far" in write mode, so a buffer that was just
 * written to can't be handed straight to buffer_getc().
 *
 * Build:
 *   cc -O2 -Wall -o test-task-buffers test-task-buffers.c \
 *     ../../../libaco/aco.c ../../../libaco/acosw.S -I../../../libaco -I../../lib \
 *     ../../build/x86_64-linux-gnu/libowfat.a
 */
#include "task.h"

#include "buffer.h"

#include <ctype.h>
#include <stdio.h>

#define N_ITEMS 3
#define SCRATCH_SIZE 128

/* stage 0: writes the input string into its own output buffer */
static void
stage_source(task_t* self, void* arg) {
  char space[SCRATCH_SIZE];
  buffer out = BUFFER_INIT_FREE(buffer_op_write, -1, space, sizeof space);
  const char* text = (const char*)arg;

  for(;;) {
    if(text == NULL)
      return;

    out.p = 0;
    buffer_puts(&out, text);

    text = (const char*)task_yield(self, &out);
  }
}

/* middle stage: reads from the buffer handed to it, uppercases each
 * byte, writes the result into its own output buffer */
static void
stage_upper(task_t* self, void* arg) {
  char space[SCRATCH_SIZE];
  buffer out = BUFFER_INIT_FREE(buffer_op_write, -1, space, sizeof space);
  buffer* in = (buffer*)arg;

  for(;;) {
    char c;

    if(in == NULL)
      return;

    out.p = 0;
    while(buffer_getc(in, &c) == 1)
      buffer_putc(&out, (char)toupper((unsigned char)c));

    in = (buffer*)task_yield(self, &out);
  }
}

/* middle stage: reads from the buffer handed to it, reverses the
 * bytes, writes the result into its own output buffer */
static void
stage_reverse(task_t* self, void* arg) {
  char space[SCRATCH_SIZE];
  char collected[SCRATCH_SIZE];
  buffer out = BUFFER_INIT_FREE(buffer_op_write, -1, space, sizeof space);
  buffer* in = (buffer*)arg;

  for(;;) {
    size_t len, i;
    char c;

    if(in == NULL)
      return;

    len = 0;
    while(len < sizeof collected && buffer_getc(in, &c) == 1)
      collected[len++] = c;

    out.p = 0;
    for(i = len; i > 0; i--)
      buffer_putc(&out, collected[i - 1]);

    in = (buffer*)task_yield(self, &out);
  }
}

/* last stage: reads from the buffer handed to it and collects the
 * final, null-terminated result */
static void
stage_sink(task_t* self, void* arg) {
  char result[SCRATCH_SIZE];
  buffer* in = (buffer*)arg;

  for(;;) {
    size_t len = 0;
    char c;

    if(in == NULL)
      return;

    while(len < sizeof result - 1 && buffer_getc(in, &c) == 1)
      result[len++] = c;
    result[len] = 0;

    in = (buffer*)task_yield(self, result);
  }
}

int
main(void) {
  task_t* stage[4];
  const char* items[N_ITEMS] = {"hello world", "task.h buffers",
                                 "shish pipeline"};
  int i, s;

  printf("task.h backend: %s\n", TASK_BACKEND_NAME);

  stage[0] = task_create(stage_source, 0);
  stage[1] = task_create(stage_upper, 0);
  stage[2] = task_create(stage_reverse, 0);
  stage[3] = task_create(stage_sink, 0);

  for(i = 0; i < N_ITEMS; i++) {
    buffer hop[2]; /* read-buffers wrapping the previous stage's output */
    void* val = (void*)items[i];

    val = task_resume(stage[0], val);
    buffer_frombuf(&hop[0], ((buffer*)val)->x, ((buffer*)val)->p);

    val = task_resume(stage[1], &hop[0]);
    buffer_frombuf(&hop[1], ((buffer*)val)->x, ((buffer*)val)->p);

    val = task_resume(stage[2], &hop[1]);
    buffer_frombuf(&hop[0], ((buffer*)val)->x, ((buffer*)val)->p);

    val = task_resume(stage[3], &hop[0]);

    printf("%-16s -> %s\n", items[i], (char*)val);
  }

  /* drain: tell every stage the stream is over so it exits its loop */
  for(s = 0; s < 4; s++)
    task_resume(stage[s], NULL);

  for(s = 0; s < 4; s++) {
    if(!task_done(stage[s])) {
      fprintf(stderr, "stage %d did not finish cleanly\n", s);
      return 1;
    }
    task_destroy(stage[s]);
  }

  return 0;
}
```

Output (identical on both backends):

```
task.h backend: libaco
hello world      -> DLROW OLLEH
task.h buffers   -> SREFFUB H.KSAT
shish pipeline   -> ENILEPIP HSIHS
```

## `build-test-task.sh`

Builds both demos for either compiler. `test-task-buffers.c` links
this repo's own already-built `libowfat.a` (native: `build/<triple>`,
picked via `$cc -dumpmachine`; wasm: `build/emscripten`) rather than
recompiling libowfat from source, so the target's `cfg-*` build has to
exist first (see `cfg-cmake.sh`). Runnable from any directory -- it
`cd`s to its own location (`experiments/task-pipeline/`) first, so
every path below is relative to there, not the repo root.

```sh
#!/bin/sh
# build-test-task.sh <gcc|emcc> -- build test-task.c and
# test-task-buffers.c against task.h's two backends. gcc links
# ../../../libaco directly; emcc needs -sASYNCIFY, nothing else.
#
# test-task-buffers.c also needs libowfat's buffer_* functions --
# reused from this repo's own build/ tree (native: build/<triple>,
# picked via `$cc -dumpmachine`; wasm: build/emscripten), so run a
# cfg-cmake.sh build for that target first if it isn't there yet.
#
# Runnable from anywhere -- cd's to its own directory first.
set -e

cd "$(dirname "$0")"

cc=$1

if [ -z "$cc" ]; then
  echo "usage: $0 <gcc|emcc>" >&2
  exit 1
fi

case $(basename "$cc") in
  emcc)
    owfat=../../build/emscripten/libowfat.a

    if [ ! -f "$owfat" ]; then
      echo "$0: $owfat not found -- run cfg-emscripten (see cfg-cmake.sh) first" >&2
      exit 1
    fi

    "$cc" -O2 -Wall -Wextra -sASYNCIFY -o test-task.js test-task.c
    echo "built test-task.js -- run with: node test-task.js"

    "$cc" -O2 -Wall -Wextra -sASYNCIFY -I../../lib -o test-task-buffers.js \
      test-task-buffers.c "$owfat"
    echo "built test-task-buffers.js -- run with: node test-task-buffers.js"
    ;;
  gcc|cc|clang)
    triple=$("$cc" -dumpmachine)
    owfat="../../build/$triple/libowfat.a"

    if [ ! -f "$owfat" ]; then
      echo "$0: $owfat not found -- run cfg (see cfg-cmake.sh) for $triple first" >&2
      exit 1
    fi

    "$cc" -O2 -Wall -Wextra -o test-task \
      test-task.c ../../../libaco/aco.c ../../../libaco/acosw.S -I../../../libaco
    echo "built test-task -- run with: ./test-task"

    "$cc" -O2 -Wall -Wextra -o test-task-buffers \
      test-task-buffers.c ../../../libaco/aco.c ../../../libaco/acosw.S \
      -I../../../libaco -I../../lib "$owfat"
    echo "built test-task-buffers -- run with: ./test-task-buffers"
    ;;
  *)
    echo "$0: unsupported compiler '$cc' (expected gcc or emcc)" >&2
    exit 1
    ;;
esac
```

Usage:

```sh
./experiments/task-pipeline/build-test-task.sh gcc     # native, links ../../../libaco
./experiments/task-pipeline/build-test-task.sh emcc    # wasm, -sASYNCIFY; run output with node
```

## Next steps, if this is pursued for real

None of this is wired into shish's evaluator yet -- `task.h` and its
demos are a standalone validation that the coroutine mechanism itself
works, portably, with real data flowing through real per-stage state.
Turning this into an actual `eval_pipeline()` execution mode needs, in
rough order of how much they block each other:

1. **Scope which stages qualify.** Only a run of *consecutive
   builtins* in a pipeline can go through `task.h` -- anything that
   execs an external program still needs a real `fork()`/`execve()`,
   same as today. A pipeline can be a mix of both (e.g. `builtin1 |
   external | builtin2`), so `eval_pipeline()`'s flatten step (see
   `pipeline-binary-refactor.md`) would need to partition stages into
   runs, not assume all-or-nothing.

2. **Shell-state isolation at each yield/resume boundary.** This is
   the load-bearing open question flagged above. `fork()` isolates
   `sh->exitcode`, variable scope, cwd, traps, `$$`/job-table entries
   per stage today, for free, via copy-on-write. Coroutines sharing
   one address space don't get that automatically -- every place that
   currently relies on "this pipeline stage has its own private
   process" needs to either (a) be proven to not actually need
   isolation for a builtin-only run (e.g. maybe cwd/traps are fine
   shared, since POSIX pipeline semantics don't guarantee subshell
   variable isolation for builtins the same way anyway -- needs
   checking against the spec, not assumed), or (b) get explicit
   save/restore at each `task_yield()`/`task_resume()`, mirroring what
   `fork()` gave away implicitly.

3. **A fiber-backed `buffer` op/fd variant.** `lib/buffer.c`'s
   `op`/`fd` pair currently always means a real syscall
   (`buffer_op_read`/`buffer_op_write`). A pipeline stage's stdin/
   stdout would need a variant whose `op` does `task_yield()`/
   `task_resume()` against the adjacent stage's `task_t` instead of a
   real `read()`/`write()`, wired in wherever `eval_pipeline.c`
   currently sets up `fd_push()`/`fd_pipe()` for a builtin-only run.

4. **A cost/benefit check against `vfork()` or posix_spawn-style
   avenues**, before committing to the state-isolation work above --
   worth confirming `fork()` overhead is actually the bottleneck this
   is solving for shish's typical pipelines, not a solution in search
   of a problem.

Recommendation: prototype step 2 in isolation first (two builtins
piped together, sharing nothing but the mailbox, deliberately *not*
touching `eval_pipeline.c` yet) to find out empirically how much shell
state actually needs saving/restoring, before scoping the rest.
