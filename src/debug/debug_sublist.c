#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../../lib/str.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"
#include "../tree.h"

/* print a subnode list
 * ----------------------------------------------------------------------- */
void
debug_sublist(const char* s, union node* node, int depth) {
  /*if(node)*/ {
    if(s)
      debug_field(s, depth);
    debug_list(node, depth >= 0 ? depth : 0);
  }

  debug_fl();
}
#endif /* DEBUG_OUTPUT */
