#include "../debug.h"
#include "../buffer.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a char
 * ----------------------------------------------------------------------- */
void
debug_char(const char* msg, char c, int depth) {
  debug_space(depth, 0);
  buffer_puts(buffer_2, COLOR_YELLOW);
  buffer_puts(buffer_2, msg);
  buffer_puts(buffer_2, COLOR_CYAN " = '");
  if(c >= 20)
    buffer_put(buffer_2, &c, 1);
  else if(c == '\n')
    buffer_put(buffer_2, "\\n", 2);
  else if(c == '\r')
    buffer_put(buffer_2, "\\r", 2);
  else if(c == '\t')
    buffer_put(buffer_2, "\\t", 2);
  else {
    /*  buffer_puts(buffer_2, "\\0");
      buffer_put8long(buffer_2, c);*/
    buffer_puts(buffer_2, "\\x");
    buffer_putxlong0(buffer_2, c, 2);
  }
  buffer_putsflush(buffer_2, "'" COLOR_NONE);
}
#endif /* defined(DEBUG_OUTPUT) */