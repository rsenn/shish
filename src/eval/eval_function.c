#include "../tree.h"
#include "../eval.h"

extern struct nfunc* functions;

/* ----------------------------------------------------------------------- */
int
eval_function(struct eval* e, struct nfunc* func) {
  int ret = 0;
  union node* fn = tree_newnode(N_FUNCTION);

  fn->nfunc.next = functions;
  functions = &fn->nfunc;
  fn->nfunc.name = func->name;
  func->name = NULL;
  fn->nfunc.body = func->body;
  func->body = NULL; 

  return ret;
}
