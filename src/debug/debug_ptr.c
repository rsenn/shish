#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"
#include "../../lib/fmt.h"

/* output a pointer
 * ----------------------------------------------------------------------- */
void
debug_ptr(const char* msg, void* ptr, int depth) {
  char buf[FMT_XLONG];
  unsigned long n;
  // debug_space(depth, 0);
  if(msg)
    debug_field(msg, depth);
  n = fmt_xlong(buf, (unsigned long)ptr);
  debug_s("0x");
  buffer_putnspace(&debug_buffer, 8 - n);
  debug_b(buf, n);
  debug_s(COLOR_NONE "\n");
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
