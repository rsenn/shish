#include "../../lib/uint64.h"
#define DEBUG_NOCOLOR 1
#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output an unsigned long
 * ----------------------------------------------------------------------- */
void
debug_ulong(const char* msg, uint64 l, int depth) {
  if(msg)
    debug_field(msg, depth);
  debug_s(COLOR_GREEN);
  debug_n(l);
  debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
