#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../tree.h"

/* ----------------------------------------------------------------------- */
void
debug_position(const char* msg, const struct position* pos, int depth) {
  if(msg)
    debug_field(msg, depth);
  debug_c('"');
  debug_n(pos->line);
  debug_s(COLOR_CYAN ":");
  debug_n(pos->column);
  debug_c('"');
  debug_fl();
}
