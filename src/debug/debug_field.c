#include "../debug.h"
#include "../fd.h"

/* print a field name, preceded by the separator its leading "," / " " asks for
 * ----------------------------------------------------------------------- */
void
debug_field(const char* s, int depth) {
  if(s[str_chr(", ", *s)]) {
    if(*s == ',')
      debug_c(',');

    if(depth >= 0)
      debug_newline(depth);
    else
      debug_c(' ');

    if(*s == ',')
      s++;

    if(*s == ' ')
      s++;
  }

  debug_c('"');
  debug_s(s);
  debug_c('"');
  debug_s(COLOR_CYAN ": ");
}
