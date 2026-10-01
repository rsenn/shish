#include "../filter.h"
#include "../../lib/buffer.h"

/* feed the whole content of path to sink() in place, block by block;
 * returns -1 if it cannot be read or encounters an error during copy.
 * ----------------------------------------------------------------------- */
int
filter_copy(const char* path, filter_sink_fn* sink, void* ctx) {
  buffer b;
  char rbuf[4096];
  ssize_t n;

  if(filter_open_file(&b, rbuf, sizeof(rbuf), path) == -1)
    return -1;

  while((n = buffer_feed(&b)) > 0) {
    sink(ctx, buffer_PEEK(&b), (size_t)n);
    buffer_SEEK(&b, (size_t)n);
  }

  buffer_close(&b);
  return n < 0 ? -1 : 0;
}
