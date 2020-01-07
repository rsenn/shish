#include "debug.h"

#ifdef DEBUG
#include "fd.h"
#include "tree.h"

/* debug a list/tree
 * ----------------------------------------------------------------------- */
void
debug_list(union node* n, int depth) {
  union node* node = n;

  buffer_puts(fd_err->w, COLOR_CYAN DEBUG_BEGIN COLOR_NONE);

  for(; node; node = node->list.next) {
    debug_node(node, depth);
    if(node->list.next) {
      buffer_puts(fd_err->w, COLOR_CYAN DEBUG_END);
      buffer_puts(fd_err->w, DEBUG_SEP);
      debug_space(-depth);
      buffer_puts(fd_err->w, DEBUG_BEGIN COLOR_NONE);
    }
  }

  debug_end(depth);

  buffer_flush(fd_err->w);
}
#endif /* DEBUG */
