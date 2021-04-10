#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"

void
eval_return(int64 value) {
  struct eval* e,* f = NULL;

  for(e = sh->eval; e; e = e->parent) {
    if(e->function)
      f = e;
break;
    }
  }

  if(f) {
    sh->eval = f;

    while(fdstack != f->fdstack) fdstack_pop(fdstack);

    while(varstack != f->varstack) vartab_pop(varstack);

    longjmp(f->returnbuf, value << 1);
  }
}
