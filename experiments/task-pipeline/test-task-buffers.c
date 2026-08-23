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
