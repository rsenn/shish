#include "../tree.h"
#include "../eval.h"

extern struct nfunc* functions;


/* ----------------------------------------------------------------------- */
int
eval_function(struct eval* e, struct nfunc* func) {
  int ret = 0;

  func->next = functions;
  functions = func;

  return ret;
}
