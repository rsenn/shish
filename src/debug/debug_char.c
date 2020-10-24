#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a char
 * ----------------------------------------------------------------------- */
void
debug_char(const char* msg, char c, int depth) {
  debug_space(depth, 0);
  buffer_puts(buffer_2, msg);
  buffer_puts(buffer_2, " = ");
  if(isprint(c))
    buffer_put(buffer_2, &c, 1);
  else {
    /*  buffer_puts(buffer_2, "\\0");
      buffer_put8long(buffer_2, c);*/
    buffer_puts(buffer_2, "\\x");
    buffer_putxlong0(buffer_2, c, 2);
  }
  buffer_putnlflush(buffer_2);
}
#endif /* defined(DEBUG_OUTPUT) */