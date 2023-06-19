#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"

void
eval_jump(int levels, int cont) {
  struct eval* e;
  struct eval* j = NULL;

  for(e = eval; e; e = e->parent) {
    if(levels <= 0)
      break;

    if(e->jump && (e->flags & E_LOOP)) {
      j = e;
      levels--;
    }
  }

  if(j) {
    eval = j;

    while(fdstack != j->fdstack)
      fdstack_pop(fdstack);
    while(varstack != j->varstack)
      vartab_pop(varstack);

    longjmp(j->jumpbuf, cont << 1);
  }
}
