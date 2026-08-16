#include "../expand.h"
#include "../tree.h"
#include "../debug.h"

/* expand an assignment list
 * ----------------------------------------------------------------------- */
int
expand_vars(union node* vars, union node** nptr) {
  union node *var, *node;
  union node* owned;
  int ret = 0;
  stralloc name;
  stralloc_init(&name);

  /* vars is the permanent parsed command tree, reused on every
     execution -- expand_tilde_assign() rewrites text in place, so it
     must only run on a private, disposable copy. */
  owned = tree_copy(vars);

  for(var = owned; var; var = var->next) {
    expand_tilde_assign(var);
    node = 0;
    node = expand_arg(var, &node, X_NOSPLIT);

    /* X_NOSPLIT routes every chunk through expand_cat()'s
       non-splitting branch, which unescapes each literal chunk as
       it's appended -- nul-terminate is all that's left to do. */
    if(node)
      stralloc_nul(&node->narg.stra);

    while(*nptr)
      tree_skip(nptr);

    *nptr = node;
    nptr = tree_next(nptr);
    ret++;
  }

  if(owned)
    tree_free(owned);

  return ret;
}
