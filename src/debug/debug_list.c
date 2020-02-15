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
  int id = n->id;

  if(id == N_ARG) {
    if(i == 0)
      buffer_puts(fd_err->w, "\n ");
      buffer_putnspace(fd_err->w, depth > 0 ? (depth -1) * 2 : 1);
  }
  for(node = n; node; i++, node = node->list.next) {
    /*
        if(depth > 0 )
      buffer_putnspace(fd_err->w, (depth ) * 2);*/
    if(id == N_ARG) {
      /*   if(i == 0)
           buffer_putnspace(fd_err->w, depth > 0 ? depth * 2 : id == N_ARG && depth != -2 ? 4 : 0);
   */
      buffer_putulong(fd_err->w, i);
      buffer_puts(fd_err->w, ": ");
    }
    debug_node(node, id >= N_ARGSTR ? -2 : id == N_ARG ? -1 : depth);
    if(node->list.next) {
      if(depth > 0 && id != N_ARG) {
        buffer_puts(fd_err->w, COLOR_CYAN DEBUG_END);
        buffer_puts(fd_err->w, DEBUG_SEP);
        debug_space(depth, 1);
        buffer_puts(fd_err->w, DEBUG_BEGIN COLOR_NONE);
      } else if(depth == -2) {
        buffer_puts(fd_err->w, ", ");
      }

      if(id == N_ARG) {
        buffer_putnspace(fd_err->w, depth );
      }
    }
  }
  if(depth > 0 && id != N_ARG && id < N_ARGSTR)
    debug_end(depth);
  buffer_flush(fd_err->w);
}
#endif /* DEBUG_OUTPUT */