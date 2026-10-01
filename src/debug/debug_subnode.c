#define DEBUG_NOCOLOR 1
#include "../debug.h"

/* print a node which is child to the current
 * ----------------------------------------------------------------------- */
void
debug_subnode(const char* s, union node* node, int depth) {
  if(node) {
    if(s)
      debug_field(s, depth);

    debug_node(node, depth >= 0 ? depth + 1 : depth);
  }
}
