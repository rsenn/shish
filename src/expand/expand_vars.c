#include "../expand.h"
#include "../tree.h"
#include "../parse.h"
#include "../debug.h"

#include <stdlib.h>

/* expand an assignment list
 * ----------------------------------------------------------------------- */
int
expand_vars(union node* vars, union node** nptr) {
  union node *var, *node;
  int ret = 0;
  stralloc name;
  stralloc_init(&name);

  for(var = vars; var; var = var->next) {
    node = 0;
    node = expand_arg(var, &node, X_NOSPLIT);

    if(node) {
      debug_node(node, 0);
      debug_nl_fl();

      expand_unescape(&node->narg.stra, parse_isesc);
    }
    if(node->id == N_ARG) {
      debug_stralloc("var", &node->narg.stra, 0, 0);
      debug_nl_fl();
    }

    while(*nptr) tree_skip(nptr);

    *nptr = node;
    nptr = tree_next(nptr);
    ret++;
  }

  return ret;
}
