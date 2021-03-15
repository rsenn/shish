#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../../lib/open.h"
#include <unistd.h>

char debug_b[1024];
buffer debug_buffer = BUFFER_INIT(&write, -1, debug_b, sizeof(debug_b));

/* begin a {}-block
 * ----------------------------------------------------------------------- */
void
debug_begin(const char* s, int depth) {

  if(debug_buffer.fd == -1)
    debug_buffer.fd = open_trunc("debug.log");

  debug_space(depth - 1, 0);
  debug_space(depth, 0);
  debug_s(COLOR_YELLOW);
  debug_s(s);
  debug_s(COLOR_CYAN DEBUG_EQU DEBUG_BEGIN COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
