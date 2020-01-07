#include "parse.h"
#include "tree.h"

/* make an argument node from the stuff parsed by parse_word
 * ----------------------------------------------------------------------- */
union node*
parse_getarg(struct parser* p) {
  union node* n;

  if(p->tree) {
    n = tree_newnode(N_ARG);
    n->narg.list = p->tree;
    p->tree = NULL;
    return n;
  }

  return NULL;
}
