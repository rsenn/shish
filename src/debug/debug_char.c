#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../../lib/buffer.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a char
 * ----------------------------------------------------------------------- */
void
debug_char(const char* msg, char c, int depth) {
  debug_s(COLOR_YELLOW);
  debug_s(msg);
  debug_s(COLOR_CYAN " = '");
  if(c >= 20)
    debug_b(&c, 1);
  else if(c == '\n')
    debug_b("\\n", 2);
  else if(c == '\r')
    debug_b("\\r", 2);
  else if(c == '\t')
    debug_b("\\t", 2);
  else {
    debug_s("\\x");
    buffer_putxlong0(&debug_buffer, c, 2);
  }
  buffer_puts(&debug_buffer, "'" COLOR_NONE);
  debug_fl();
}
#endif /* defined(DEBUG_OUTPUT) */
