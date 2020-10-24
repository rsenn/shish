#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_unquoted(const char* msg, const char* s, int depth) {

  if(msg) {
    buffer_puts(buffer_2, COLOR_YELLOW);
    buffer_puts(buffer_2, msg);
    buffer_puts(buffer_2, COLOR_CYAN DEBUG_EQU);
  }
  buffer_puts(buffer_2, COLOR_RED);
  buffer_puts(buffer_2, s);
  buffer_puts(buffer_2, COLOR_NONE);
  // buffer_flush(buffer_2);
}
#endif /* DEBUG_OUTPUT */
