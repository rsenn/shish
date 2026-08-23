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
