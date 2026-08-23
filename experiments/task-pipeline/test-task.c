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
