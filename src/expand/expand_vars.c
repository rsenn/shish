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

  /* vars is ncmd->vars, the permanent parsed command tree -- same
     reasoning as expand_args.c: expand_tilde_assign() rewrites a
     word's text in place, so it must only ever run on a private,
     disposable copy, never the original (reused on every execution of
     this command node). */
  owned = tree_copy(vars);

  for(var = owned; var; var = var->next) {
    expand_tilde_assign(var);
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

  if(owned)
    tree_free(owned);

  return ret;
}
