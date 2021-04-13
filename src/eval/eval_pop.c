#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"
#include <assert.h>

int
eval_pop(struct eval* e) {
  int ret;

  assert(e == sh->eval);

  ret = sh->eval->exitcode;

  while(fdstack != e->fdstack && &fdstack_root != fdstack) fdstack_pop(fdstack);

  while(varstack != e->varstack && &vartab_root != varstack) vartab_pop(varstack);

  sh->exitcode = e->exitcode;
  sh->eval = e->parent;
  eval = e->parent;

  return ret;
}
