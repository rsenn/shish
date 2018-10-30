#include "sh.h"
#include "fdstack.h"
#include "vartab.h"
#include "tree.h"
#include "eval.h"

/* evaluate subshell or grouping (3.9.4.1)
 * ----------------------------------------------------------------------- */
int eval_subshell(struct eval *e, struct ngrp *ngrp) {
  int ret;
  struct env sh;
  struct fdstack io;
  struct vartab vars;

  fdstack_push(&io);
  vartab_push(&vars);
  sh_push(&sh);
  
  ret = sh_subshell(ngrp->cmds, e->flags);

  sh_pop(&sh);
  vartab_pop(&vars);
  fdstack_pop(&io);

  return ret;
}

