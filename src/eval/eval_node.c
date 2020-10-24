#include "../fd.h"
#include "../eval.h"
#include "../sh.h"
#include "../tree.h"

/* evaluate a tree node(-list maybe)
 * ----------------------------------------------------------------------- */
int
eval_node(struct eval* e, union node* node) {
  int ret = -1;
  switch(node->id) {
    case N_SIMPLECMD: {
      ret = eval_simple_command(e, &node->ncmd);
      break;
    }
    case N_FUNCTION: {
      ret = eval_function(e, &node->nfunc);
      break;
    }
    case N_CMDLIST: {
      ret = eval_cmdlist(e, &node->ngrp);
      break;
    }
    case N_IF:
    case N_FOR:
    case N_CASE:
    case N_WHILE:
    case N_UNTIL:
    case N_SUBSHELL: {
      ret = eval_command(e, node, 0);
      break;
    }
    case N_PIPELINE: {
      ret = eval_pipeline(e, &node->npipe);
      break;
    }
    case N_AND:
    case N_OR:
    case N_NOT: {
      ret = eval_and_or(e, &node->nandor);
      break;
    }
    default: {
      ret = 0;
      break;
    }
  }

  return ret;
}