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
  /* if(id <  N_ARGSTR || id == N_ARG) {
      debug_space(depth + 1, id == N_ARG);
    }*/
  for(node = n; node; i++, node = node->list.next) {
    if(i == 0 && id < N_ARGSTR)
      buffer_puts(buffer_2, COLOR_CYAN "[" COLOR_NONE);

    debug_space(depth + 1, depth > 0);
    if(id == N_ARG) {
      buffer_putulong(buffer_2, i);
      buffer_puts(buffer_2, ":");
    } else if(id >= N_ARGSTR) {
      buffer_puts(buffer_2, i == 0 ? (COLOR_CYAN "{" COLOR_NONE) : ", ");
    }
    debug_node(node, id == N_ARG ? -2 : depth > 0 ? depth + 1 : depth);
    if(node->list.next) {
      if(id == N_ARG) {
        debug_space(depth - 1, 0);
      } else if(id >= N_ARGSTR) {
      } else if(depth > 0) {
        buffer_puts(buffer_2, COLOR_CYAN DEBUG_END);
        buffer_puts(buffer_2, DEBUG_SEP);
        debug_space(depth, id == N_ARG ? 0 : depth > 0);
        buffer_puts(buffer_2, DEBUG_BEGIN COLOR_NONE);
      }
    }
  }
  if(id >= N_ARGSTR) {
    debug_space(depth - 1, 0);
    buffer_puts(buffer_2, id == N_ARG ? " " : (COLOR_CYAN "}" COLOR_NONE));
  } else if(depth > 0) {
    debug_space(depth, id == N_ARG);
    if(id != N_ARG && id < N_ARGSTR)
      debug_end(depth);
    buffer_flush(buffer_2);
  }
}
#endif /* DEBUG_OUTPUT */
