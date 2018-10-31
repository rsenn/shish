#include "sh.h"
#include "source.h"
#include "fdtable.h"
#include "vartab.h"
#include "history.h"

/* exits current subshell, never returns!
 * ----------------------------------------------------------------------- */
void sh_exit(int retcode) {
  /* we're in a subshell, jump back where we established it */
  if(sh->jump) {
/*    while(fdstack != sh->fdtable)
      fdtable_pop(fdstack);
    while(varstack != sh->vartab)
      vartab_pop(varstack);*/
    
    longjmp(sh->jmpbuf, (retcode << 1) | 1);
  }
  /* not in a subshell, exit the process */
  else {
    if(sh == &sh_root && source)
      source_pop();

    exit(retcode);
  }
}

