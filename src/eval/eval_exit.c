#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"

void
eval_exit(int exitcode) {
  struct env* parent_sh = sh->parent;
  struct eval* e;

  for(e = sh->eval; e && e != parent_sh->eval; e = e->parent) {
    if(e == parent_sh->eval)
      return;

    if(e->jump && (e->flags & E_ROOT))
      break;
  }

  if(e) {
    e->exitcode = exitcode;

    /*    while(fdstack != e->fdstack) fdstack_pop(fdstack);
        while(varstack != e->varstack) vartab_pop(varstack);
    */
    longjmp(e->jumpbuf, (exitcode << 1) | 1);
  }
}
