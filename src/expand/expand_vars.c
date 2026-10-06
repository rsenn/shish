#include "../expand.h"
#include "../tree.h"
#include "../debug.h"

/* expand an assignment list: one "name=value" field per assignment, never split
 * returns the number of assignments
 * ----------------------------------------------------------------------- */
int
expand_vars(union node* vars, wordlist* wl) {
  union node *var, *owned;
  int ret = 0, copied;

  /* vars is the permanent parsed command tree, reused on every
     execution -- expand_tilde_assign() rewrites text in place, so it
     must only run on a private, disposable copy. */
  for(var = vars; var; var = var->next)
    if(expand_tilde_assign_needed(var))
      break;

  copied = var != NULL;
  owned = copied ? tree_copy(vars) : vars;

  for(var = owned; var; var = var->next) {
    if(copied)
      expand_tilde_assign(var);

    expand_arg(var, wl, X_NOSPLIT);

    /* an assignment is never pathname-expanded */
    wl->state &= ~(X_GLOB | X_GLOBRES);
    wordlist_close(wl);
    ret++;
  }

  if(copied && owned)
    tree_free(owned);

  return ret;
}
