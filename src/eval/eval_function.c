#include "../../lib/str.h"
#include "../tree.h"
#include "../eval.h"
#include "../exec.h"

extern union node* functions;

static inline union node**
find_function(const char* name) {
  union node** nptr = &functions;

  for(nptr = &functions; *nptr; nptr = tree_next(nptr)) {
    struct nfunc* fn = &(*nptr)->nfunc;

    if(!str_diff(fn->name, name))
      return nptr;
  }

  return 0;
}

/* ----------------------------------------------------------------------- */
int
eval_function(struct eval* e, struct nfunc* func) {
  int ret = 0;
  union node *fn, **nptr;

  if((nptr = find_function(func->name))) {
    fn = *nptr;
    *nptr = (*nptr)->next;
    /* IMPORTANT: detach fn from the rest of the list before freeing.
       tree_free walks via node->next, so without this the call below
       would cascade and free every function that was defined earlier
       than this one (as_fn_exit, as_fn_unset, ... all gone when
       configure redefines as_fn_nop). */
    fn->next = NULL;
    /* While a subshell is evaluating, the parent shell's func_snapshot may
       still reference this node. Leak it for now (orphaned but safe);
       exec_functions_restore will relink the parent's list on exit. */
    if(exec_subshell_depth == 0)
      tree_free(fn);
  }

  /* Invalidate any cached exec_hash entry for this name. The cache holds
     cmd.fn = body pointer, which we either just freed (redefine) or are
     about to replace. Without this, autoconf's `. ./$as_me.lineno` self-
     source resets all functions but the cache still serves the stale body
     pointer, producing a use-after-free on the next invocation. */
  {
    uint32 h;
    struct exechash* e = exec_lookup(func->name, &h);
    if(e)
      e->mask = -1; /* force exec_search re-run on next lookup */
  }

  fn = tree_newnode(N_FUNCTION);

  fn->next = functions;
  fn->nfunc.name = func->name;
  fn->nfunc.body = func->body;
  func->name = NULL;
  func->body = NULL;

  functions = fn;

  return ret;
}
