#include "../builtin.h"
#include "../../lib/buffer.h"
#include "../../lib/str.h"

/* open the next file operand in sequence; returns 0 when there are
 * no operands left to process.
 * ----------------------------------------------------------------------- */
int
filter_in_next(struct filter_in* in) {
  for(;;) {
    const char* name = "-";

    filter_in_close(in);

    if(in->files) {
      if(!(name = in->files[in->i]))
        return 0; /* no more operands left */

      in->i++;
    } else if(in->done_any) {
      return 0; /* stdin already consumed when no explicit files given */
    }

    in->done_any = in->newfile = 1;

    /* if name is "-", bind to upstream buffer source */
    if(!str_diff(name, "-")) {
      in->cur = in->upstream;
      return 1;
    }

    /* try opening the physical file path */
    if(filter_open_file(&in->inb, in->rbuf, sizeof(in->rbuf), name) == 0) {
      in->cur = &in->inb;
      return 1;
    }

    /* report error if file opening fails and continue to next */
    builtin_error(in->errargv, (char*)name);
    in->had_error = 1;
  }
}
