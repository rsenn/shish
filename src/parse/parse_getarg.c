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

#if DEBUG_PARSE
    buffer_puts(fd_err->w, COLOR_YELLOW "parse_getarg" COLOR_NONE " = ");
    tree_print(n, fd_err->w);
    buffer_putnlflush(fd_err->w);
#endif
    return n;
  }

  return NULL;
}
