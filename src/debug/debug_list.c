#include "../../lib/uint64.h"
#define DEBUG_NOCOLOR 1
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

  debug_begin(0, depth >= 0 ? depth + 1 : depth);

  for(node = n; node; i++, node = node->next) {
    /*  if(id == N_ARG) {
      } else if(id >= N_ARGSTR) {
        debug_s(i == 0 ? (COLOR_CYAN DEBUG_BEGIN COLOR_NONE) : ", ");
      }*/
    debug_node(node, depth >= 0 ? depth + 2 : depth);
    if(node->next) {
      /*  if(id == N_ARG) {
        } else if(id >= N_ARGSTR) {
        } else*/
      {
        int n;
        debug_c(',');
        debug_newline(depth);

        //        debug_s(COLOR_CYAN DEBUG_SEP COLOR_NONE);
      }
    }
  }
  debug_end(depth);
}
#endif
