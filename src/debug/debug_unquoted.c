#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_unquoted(const char* msg, const char* s, int depth) {

  if(msg) {
    debug_s(COLOR_YELLOW);
    debug_s(msg);
    debug_s(COLOR_CYAN DEBUG_EQU);
  }
  debug_s(COLOR_RED);
  debug_s(s);
  debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
