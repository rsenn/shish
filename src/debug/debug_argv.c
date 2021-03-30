#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../parse.h"
#include "../../lib/str.h"

/* ----------------------------------------------------------------------- */
size_t
debug_argv(char** argv, buffer* out) {
  char** arg;
  size_t i;

  i = out->p;

  for(arg = argv; *arg; arg++) {
    int quote = !!(*arg)[str_chr(*arg, ' ')];

    if(!quote) {
      char* s;
      for(s = *arg; *s; s++)
        if(parse_isctrl(*s)) {
          quote++;
          break;
        }
    }

    if(arg > argv)
      buffer_putspace(out);
    if(quote)
      buffer_putc(out, '\'');
    buffer_puts(out, *arg);
    if(quote)
      buffer_putc(out, '\'');
  }
  return out->p - i;
}
