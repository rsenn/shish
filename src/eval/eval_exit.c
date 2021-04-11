#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"

void
eval_exit(int exitcode) {
  struct eval *e;

  for(e = sh->eval; e; e = e->parent) {
    if(e->jump && (e->flags & E_ROOT))
       break;
  }

  if(e) {
    sh->eval = e;

    while(fdstack != e->fdstack) fdstack_pop(fdstack);
    while(varstack != e->varstack) vartab_pop(varstack);

    longjmp(e->jumpbuf, (exitcode << 1) | 1);
  }
}
