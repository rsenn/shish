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
      if(node->narg.flag & X_LITERAL)
        expand_unescape(&node->narg.stra, parse_isesc);
      else
        /* expand_unescape() nul-terminates as a side effect -- see
           expand_args.c's identical comment */
        stralloc_nul(&node->narg.stra);
    }

    while(*nptr)
      tree_skip(nptr);

    *nptr = node;
    nptr = tree_next(nptr);
    ret++;
  }

  return ret;
}
