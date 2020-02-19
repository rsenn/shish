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
    if(s) {
      buffer_puts(buffer_2, COLOR_YELLOW);
      buffer_puts(buffer_2, s);
      buffer_puts(buffer_2, COLOR_CYAN "=" COLOR_NONE);
      /*
            if(!str_diffn(s, "cmd", 3)) {
              buffer_puts(buffer_2, COLOR_CYAN "[" COLOR_NONE);
            }*/
    }
    debug_list(node, depth < 0 ? depth : depth + 1);

  } else {
    debug_unquoted(s, COLOR_CYAN DEBUG_BEGIN DEBUG_END, depth);
  }
}
#endif /* DEBUG_OUTPUT */
