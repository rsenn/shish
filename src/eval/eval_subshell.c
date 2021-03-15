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
  int ret;
  struct env sh;
  struct fdstack io;
  struct vartab vars;

  fdstack_push(&io);
  vartab_push(&vars, 0);
  sh_push(&sh);

  ret = sh_subshell(ngrp->cmds, e->flags);

  sh_pop(&sh);
  vartab_pop(&vars);
  fdstack_pop(&io);

  return ret;
}
