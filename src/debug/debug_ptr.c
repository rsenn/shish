#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"
#include "../../lib/uint64.h"
#include "../../lib/fmt.h"
#include "../../lib/typedefs.h"

/* output a pointer
 * ----------------------------------------------------------------------- */
void
debug_ptr(const char* msg, void* ptr, int depth) {
  char buf[FMT_XLONG];
  size_t n;
  // debug_space(depth, 0);

  if(msg)
    debug_field(msg, depth);
  n = fmt_xlonglong(buf, (uint64)(size_t)ptr);
  debug_s("0x");
  buffer_putnspace(debug_output, 8 - n);
  debug_b(buf, n);
  debug_s(COLOR_NONE "\n");
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
