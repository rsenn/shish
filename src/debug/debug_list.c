#include "../../lib/uint64.h"
#define DEBUG_NOCOLOR 1
#include "../debug.h"
#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../tree.h"

/* debug a list/tree
 * ----------------------------------------------------------------------- */
void
debug_list(union node* n, int depth) {
  unsigned long i = 0;
  union node* node;
  debug_begin(0, depth >= 0 ? depth + 1 : depth);
  // debug_indent(1);
  // debug_s(DEBUG_BEGIN);
  // debug_newline(depth);
  for(node = n; node; i++, node = node->next) {
    debug_node(node, depth >= -1 ? depth + 2 : depth);
    if(node->next) {
      debug_c(',');
      debug_newline(depth + 1);
    }
  }
  debug_end(depth);
}
#endif
