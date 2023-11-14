#include "../debug.h"
#include "../../lib/byte.h"

/* ----------------------------------------------------------------------- */
void
debug_squoted(const char* x, size_t n, buffer* out) {

  size_t i, next;

  buffer_putc(out, '\'');

  for(i = 0; i < n; i += next) {
    if(x[i] == '\'') {
      buffer_puts(out, "'\\''");
      next = 1;
      continue;
    }

    next = byte_chr(&x[i], n - i, '\'');

    if(next > 0)
      buffer_put(out, &x[i], next);
  }

  buffer_putc(out, '\'');
}
