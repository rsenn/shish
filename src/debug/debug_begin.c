#define DEBUG_NOCOLOR 1
#include "../debug.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

char debug_b[1024];
buffer debug_buffer = BUFFER_INIT(&write, -1, debug_b, sizeof(debug_b));
buffer* debug_output = &debug_buffer;

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"

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
