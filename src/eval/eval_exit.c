#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"

void
eval_exit(int exitcode) {
  struct eval *e, *parent_eval = sh->parent ? sh->parent->eval : 0;

  for(e = sh->eval; e; e = e->parent) {
    if(e == parent_eval)
      return;

    if(e->jump && (e->flags & E_ROOT))
      break;
  }

  if(e) {
    if(e->destructor)
      exitcode = e->destructor(exitcode);

    e->exitcode = exitcode;

    /*while(fdstack != e->fdstack) fdstack_pop(fdstack);
    while(varstack != e->varstack) vartab_pop(varstack);*/

    longjmp(e->jumpbuf, (exitcode << 1) | 1);
  }
}
