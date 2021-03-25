#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../../lib/str.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../tree.h"

/* print a subnode list
 * ----------------------------------------------------------------------- */
void
debug_sublist(const char* s, union node* node, int depth) {
  if(node) {
    if(s)
      debug_field(s, depth);
    debug_list(node, depth);
  }
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
