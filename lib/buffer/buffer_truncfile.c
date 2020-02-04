#include "../buffer.h"
#include "../open.h"
#include "../windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>

int
buffer_truncfile(buffer* b, const char* fn) {
  if((b->fd = open_trunc(fn)) == -1)
    return -1;
  b->p = 0;
  b->n = 0;
  b->a = BUFFER_OUTSIZE;
  b->x = malloc(b->a);
  b->op = (buffer_op_proto*)write;
  b->deinit = buffer_free;
  return 0;
}
