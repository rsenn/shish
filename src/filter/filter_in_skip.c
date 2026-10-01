#include "../filter.h"
#include "../../lib/buffer.h"

/* consume n bytes previously exposed by filter_in_peek() */
void
filter_in_skip(struct filter_in* in, size_t n) {
  if(in->cur)
    buffer_SEEK(in->cur, n);
}
