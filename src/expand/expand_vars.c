#include "../expand.h"
#include "../tree.h"
#include "../debug.h"

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

    /* expand_arg() is called with X_NOSPLIT above, which always routes
       every chunk through expand_cat()'s non-splitting branch -- that
       branch now unescapes each literal chunk itself as it's appended
       (fixes/70), so the buffer here is already final; nul-terminate
       is all that's left to do (expand_unescape() used to do both as
       one unconditional call, back when this ran a second, redundant,
       whole-buffer pass over content expand_cat() had already fixed). */
    if(node)
      stralloc_nul(&node->narg.stra);

    while(*nptr)
      tree_skip(nptr);

    *nptr = node;
    nptr = tree_next(nptr);
    ret++;
  }

  return ret;
}
