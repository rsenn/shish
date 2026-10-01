#include "../filter.h"

/* zero-copy read: expose the active source's buffered bytes in place.
 * ----------------------------------------------------------------------- */
ssize_t
filter_in_peek(struct filter_in* in, const char** p) {
  if(!filter_in_ready(in))
    return 0;

  *p = buffer_PEEK(in->cur);
  return (ssize_t)buffer_LEN(in->cur);
}
