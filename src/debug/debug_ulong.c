#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output an unsigned long
 * ----------------------------------------------------------------------- */
void
debug_ulong(const char* msg, unsigned long l, int depth) {

  if(msg) {
    buffer_puts(buffer_2, COLOR_YELLOW);
    buffer_puts(buffer_2, msg);
    buffer_puts(buffer_2, COLOR_NONE COLOR_CYAN " = ");
  }
  buffer_puts(buffer_2, COLOR_GREEN);
  buffer_putulong(buffer_2, l);
  buffer_puts(buffer_2, COLOR_NONE);
  buffer_flush(buffer_2);
}
#endif /* DEBUG_OUTPUT */
