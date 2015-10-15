#include "tree.h"
#include "eval.h"

/* this could be done in tree.c */
int eval_cmdlist(struct eval *e, struct ngrp *ngrp) {
  return eval_tree(e, ngrp->cmds, E_LIST);
}

