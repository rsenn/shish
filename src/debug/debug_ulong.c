#include "../../lib/uint64.h"
#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output an unsigned long
 * ----------------------------------------------------------------------- */
void
debug_ulong(const char* msg, unsigned long l, int depth) {

  if(msg) {
    debug_s(COLOR_YELLOW);
    debug_s(msg);
    debug_s(COLOR_NONE COLOR_CYAN " = ");
  }
  debug_s(COLOR_GREEN);
  debug_n(l);
  debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
