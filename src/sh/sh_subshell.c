#include "tree.h"
#include "eval.h"
#include "sh.h"
#include "fdtable.h"

/* execute a tree in a subshell
 * 
 * you have to push stuff yourself! 
 * ----------------------------------------------------------------------- */
int sh_subshell(union node *list, int flags) {
  int ret = 0;
  int jmpret;
  struct eval e;

  eval_push(&e, E_ROOT);
  /* set up a long jump so we can exit the subshell and end up just
     after the setjmp call, which will return nonzero in this case */
  sh->jump = 1;
  jmpret = setjmp(sh->jmpbuf);

  if(jmpret) {
    ret = (jmpret >> 1);
  } else {
    eval_tree(&e, list, E_LIST);
  }

  ret = eval_pop(&e);
  
  sh->jump = 0;
  return ret;
}

