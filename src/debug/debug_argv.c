#include "../debug.h"
#include "../../lib/str.h"

/* ----------------------------------------------------------------------- */
void
debug_argv(const char** argv, buffer* out) {
  const char** arg;
  for(arg = argv; *arg; arg++) {
    int quote = !!(*arg)[str_chr(*arg, ' ')];
    if(arg > argv)
      buffer_putspace(out);
    if(quote)
      buffer_putc(out, '\'');
    buffer_puts(out, *arg);
    if(quote)
      buffer_putc(out, '\'');
  }
}
