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

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHFORMAT) && !defined(SHPARSE2AST)
    debug_begin("parse_getarg", 0);
    tree_print(n, &debug_buffer);
    debug_nl_fl();
#endif
    return n;
  }

  return NULL;
}
