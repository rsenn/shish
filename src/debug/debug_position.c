
#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../tree.h"
#include "../source.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)

int debug_emit_loc = 1;
int debug_emit_range = 0;

/* a [start, end) byte-offset pair into the whole script -- no line/col,
 * no filename, just the two numbers -- for tooling that wants to slice
 * the original source text directly instead of re-deriving an offset
 * from a "file:line:col" string.
 * ----------------------------------------------------------------------- */
void
debug_range(const char* msg, size_t start, size_t end, int depth) {
  if(msg)
    debug_field(msg, depth);
  debug_c('[');
  debug_n(start);
  debug_c(',');
  debug_n(end);
  debug_c(']');
  debug_fl();
}

/* ----------------------------------------------------------------------- */
void
debug_position(const char* msg, const struct location* pos, int depth) {
  if(msg)
    debug_field(msg, depth);
  debug_s("{");

  debug_ulong("line", pos->line, -1);
  debug_ulong(", col", pos->column, -1);
  debug_ulong(", offset", pos->offset, -1);

  debug_newline(-1);
  debug_s("}");
  debug_fl();
}

void
debug_location(const char* msg, const struct location* pos, int depth) {
  char buf[FMT_LOC];

  buf[fmt_location(buf, *pos)] = '\0';
  debug_str(msg, buf, depth, '"');
  debug_fl();
}
#endif
