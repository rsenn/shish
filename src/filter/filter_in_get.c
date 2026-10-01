#include "../filter.h"
#include "../../lib/buffer.h"

/* copying read: up to len bytes, stopping after a byte from delims.
 * ----------------------------------------------------------------------- */
ssize_t
filter_in_get(struct filter_in* in, char* buf, size_t len, const char* delims, size_t ndelims) {
  while(filter_in_ready(in)) {
    ssize_t r = buffer_get_until(in->cur, buf, len, delims, ndelims);

    if(r > 0)
      return r;

    if(r < 0)
      in->had_error = 1;

    filter_in_close(in);
  }

  return 0;
}
