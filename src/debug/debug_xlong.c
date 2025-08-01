#include "../../lib/uint64.h"
#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"

/* output an unsigned long
 * ----------------------------------------------------------------------- */
void
debug_xlong(const char* msg, uint64 l, int depth) {
  if(msg)
    debug_field(msg, depth);
  debug_s(COLOR_GREEN);
  debug_s("\"0x");
  debug_xn(l);
  debug_s("\"");
  debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
