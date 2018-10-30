#include "debug.h"

#ifdef DEBUG
#include "tree.h"
#include "fd.h"

/* print a subnode list
 * ----------------------------------------------------------------------- */
void debug_sublist(const char *s, union node *node, int depth) {
  if(node) {
    buffer_putm(fd_err->w, COLOR_YELLOW, s, COLOR_CYAN, DEBUG_EQU, NULL);
    debug_list(node, depth + 1);
  } else {
    debug_unquoted(s, COLOR_CYAN DEBUG_BEGIN DEBUG_END, depth);
  }
}
#endif /* DEBUG */
