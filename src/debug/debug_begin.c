#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"
#include <unistd.h>

char debug_b[1024];
buffer debug_buffer = BUFFER_INIT(&write, -1, debug_b, sizeof(debug_b));

/* begin a {}-block
 * ----------------------------------------------------------------------- */
void
debug_begin(const char* s, int depth) {
  debug_open();
  if(s)
    debug_s(s);
  /* else
       debug_indent(depth);
 */
  debug_s(COLOR_CYAN DEBUG_BEGIN COLOR_NONE);
  debug_newline(depth);
}
#endif /* DEBUG_OUTPUT */
