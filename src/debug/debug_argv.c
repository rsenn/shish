#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../parse.h"
#include "../../lib/str.h"

/* one word as "set -x" shows it: quoted only when it is empty or has a special character
 * ----------------------------------------------------------------------- */
void
debug_word(const char* s, size_t n, buffer* out) {
  size_t i;
  int quote = n == 0;

  for(i = 0; i < n && !quote; i++)
    quote = parse_isctrl(s[i]) || parse_isesc(s[i]) || s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\'' ||
            s[i] == '"' || s[i] == '$' || s[i] == '`' || s[i] == '<' || s[i] == '>';

  if(quote)
    debug_squoted(s, n, out);
  else
    buffer_put(out, s, n);
}

/* ----------------------------------------------------------------------- */
size_t
debug_argv(char** argv, buffer* out) {
  char** arg;
  size_t i = out->p;

  for(arg = argv; *arg; arg++) {
    int quote = **arg == '\0';
    
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
