#include "../parse.h"
#include "../tree.h"
#include "../fd.h"
#include "../debug.h"

/* make an argument node from the stuff parsed by parse_word
 * ----------------------------------------------------------------------- */
union node*
parse_getarg(struct parser* p) {
  union node* n;

  if(p->node || p->tree) {
    n = tree_newnode(N_ARG);
    n->narg.list = p->tree;
    p->tree = NULL;

    /* #if DEBUG_OUTPUT_
        buffer_puts(fd_err->w, "parse_getarg: ");
        debug_node(n, -2);
        buffer_putnlflush(fd_err->w);
    #endif */
    return n;
  }

  return NULL;
}
