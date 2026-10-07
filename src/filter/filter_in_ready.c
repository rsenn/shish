#include "../filter.h"
#include "../../lib/byte.h"
#include "../../lib/buffer.h"

/* make sure the active source has buffered bytes, cycling file operands
 * on eof. returns 0 once every source is exhausted.
 * ----------------------------------------------------------------------- */
int
filter_in_ready(struct filter_in* in) {
  in->empty = 0;

  for(;;) {
    ssize_t r;

    if(!in->cur && !filter_in_next(in))
      return 0;

    if((r = buffer_feed(in->cur)) > 0)
      return 1;

    if(r < 0)
      in->had_error = 1;
    else if(in->keepempty && in->newfile) {
      in->empty = 1; /* a file without a byte: head/tail still give it a header */
      return 1;
    }

    filter_in_close(in);
  }
}
