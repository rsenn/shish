#include "../filter.h"
#include "../../lib/buffer.h"
#include "../../lib/byte.h"

/* one whole line of any length: the newline is looked for in the buffered
 * window; a line that ends inside it is returned in place, one that does
 * not is collected in the spill buffer until it does (or the operand ends).
 * ----------------------------------------------------------------------- */
ssize_t
filter_in_line(struct filter_in* in, const char** p, int* had_nl) {
  in->spill_len = 0;

  for(;;) {
    const char* w;
    size_t n, i;

    if(in->spill_len) {
      /* mid-line: the operand ending (or every source ending) ends the line */
      int before = in->i;

      in->spilling = 1;
      n = (size_t)filter_in_ready(in);
      in->spilling = 0;

      if(!n || in->i != before)
        break;
    } else if(!filter_in_ready(in)) {
      return -1;
    }

    w = buffer_PEEK(in->cur);
    n = buffer_LEN(in->cur);
    i = byte_chr(w, n, '\n');

    if(i < n) {
      *had_nl = 1;
      buffer_SEEK(in->cur, i + 1);

      if(!in->spill_len) {
        *p = w;
        return (ssize_t)i;
      }

      if(!filter_in_spill(in, w, i)) {
        in->had_error = 1;
        return -1;
      }

      *p = in->spill;
      return (ssize_t)in->spill_len;
    }

    if(!filter_in_spill(in, w, n)) {
      in->had_error = 1;
      return -1;
    }

    buffer_SEEK(in->cur, n);
  }

  *had_nl = 0;
  *p = in->spill;
  return (ssize_t)in->spill_len;
}
