#include "../../lib/str.h"
#include "../tree.h"
#include "../eval.h"

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
    tree_free(fn);
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
