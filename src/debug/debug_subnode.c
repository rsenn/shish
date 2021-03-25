#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)

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
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
