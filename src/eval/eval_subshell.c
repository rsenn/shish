#include "../fd.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../tree.h"
#include "../vartab.h"

/* evaluate subshell or grouping (3.9.4.1)
 * ----------------------------------------------------------------------- */
int
eval_subshell(struct eval* e, struct ngrp* ngrp) {
  int ret, jmpret;
  struct eval en;
  struct env she;
  struct fdstack io;
  struct vartab vars;

  fdstack_push(&io);
  vartab_push(&vars, 0);
  sh_push(&she);

  eval_push(&en, E_ROOT);

  /* set up a long jump so we can exit the subshell and end up just
     after the setjmp call, which will return nonzero in this case */
  en.jump = 1;
  jmpret = setjmp(en.jumpbuf);

  if(jmpret) {
    en.exitcode = (jmpret >> 1);
  } else {
    eval_tree(&en, ngrp->cmds, E_LIST);
  }

  ret = eval_pop(&en);

  sh_pop(&she);
  vartab_pop(&vars);
  fdstack_pop(&io);

  sh->exitcode = ret;

  return ret;
}
