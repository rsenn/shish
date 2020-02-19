#include "../tree.h"
#include "../eval.h"

extern struct nfunc* functions;

/* ----------------------------------------------------------------------- */
int
eval_function(struct eval* e, struct nfunc* func) {
  int ret = 0;
  union node* fn = tree_newnode(N_FUNCTION);

  fn->nfunc.next = functions;
  fn->nfunc.name = func->name;
  fn->nfunc.body = func->body;
  func->name = NULL;
  func->body = NULL;

  functions = &fn->nfunc;

  return ret;
}
