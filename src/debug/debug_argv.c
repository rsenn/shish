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
    int quote = 0;
    // parse_isesc(*arg); //!!(*arg)[str_chr(*arg, ' ')];

    if(!quote) {
      char* s;
      for(s = *arg; *s; s++)
        if(parse_isctrl(*s) || parse_isesc(*s)) {
          quote++;
          break;
        }
    }

    if(arg > argv)
      buffer_putspace(out);
    if(quote) {
      char* s;
      size_t next;

      buffer_putc(out, '\'');
      for(s = *arg; *s; s += next) {
        if(*s == '\'') {
          buffer_puts(out, "'\\''");
          next = 1;
          continue;
        }

        next = str_chr(s, '\'');
        if(next > 0)
          buffer_put(out, s, next);
      }

      buffer_putc(out, '\'');
    } else {
      buffer_puts(out, *arg);
    }
  }
  return out->p - i;
}
