#define DEBUG_NOCOLOR 1
#include "../debug.h"

char debug_b[1024];
buffer debug_buffer = BUFFER_INIT(&buffer_op_write, -1, debug_b, sizeof(debug_b));
buffer* debug_output = &debug_buffer;

#include "../fd.h"

/* begin a {}-block
 * ----------------------------------------------------------------------- */
void
debug_begin(const char* s, int depth) {
  debug_open();

  if(s)
    debug_s(s);

  debug_s(COLOR_CYAN DEBUG_BEGIN COLOR_NONE);
  debug_newline(depth);
}
