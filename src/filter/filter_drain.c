#include "../filter.h"
#include "../../lib/buffer.h"

/* run a step function to completion, writing every unit to out. ctx starts
 * with its struct filter_in; output is flushed only when the next read would
 * have to wait (nothing left buffered in the active source), so an mmapped
 * file goes out in full buffers while a slow pipe or a terminal stays live.
 * ----------------------------------------------------------------------- */
void
filter_drain(filter_step_fn* step, void* ctx, buffer* out) {
  const struct filter_in* in = ctx;
  const char* unit;
  size_t len;

  while(step(ctx, &unit, &len)) {
    if(len) /* a zero-length unit needs no write; some fds' buffers have no backing array yet */
      buffer_put(out, unit, len);

    if(!in->cur || !buffer_LEN(in->cur))
      buffer_flush(out);
  }

  buffer_flush(out);
}
