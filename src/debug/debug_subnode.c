#include "../debug.h"

#ifdef DEBUG_OUTPUT

/* print a node which is child to the current
 * ----------------------------------------------------------------------- */
void
debug_subnode(const char* s, union node* node, int depth) {
  if(node) {
    debug_begin(s, depth);
    debug_node(node, depth + 1);
    debug_end(depth);
  } else {
    debug_str(s, NULL, depth, '"');
  }
}
#endif /* DEBUG_OUTPUT */
