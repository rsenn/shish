#include "tree.h"
#include "eval.h"
#include "sh.h"

/* evaluate a tree node(-list maybe)
 * ----------------------------------------------------------------------- */
int eval_tree(struct eval *e, union node *node, int tempflags) {
  int ret = 0;
  int list = 0, ex = 0;
  int oldflags;

  if((e->flags | tempflags) & E_LIST) {
    list = 1;
    e->flags &= ~E_LIST;
    tempflags &= ~E_LIST;
  }

  if((e->flags | tempflags) & E_EXIT) {
    ex = 1;
    e->flags &= ~E_EXIT;
    tempflags &= ~E_EXIT;
  }

  oldflags = e->flags;
  e->flags |= tempflags;

  while(node) {
    /* not the last node, disable E_EXIT for now */
    if(ex && (!list || node->list.next == NULL))
      e->flags |= E_EXIT;

    switch(node->id) {
    case N_SIMPLECMD:
      ret = eval_simple_command(e, &node->ncmd);
      break;

    case N_IF:
    case N_FOR:
    case N_CASE:
    case N_WHILE:
    case N_UNTIL:
    case N_SUBSHELL:
    case N_CMDLIST:
      ret = eval_command(e, node, 0);
      break;

    /*        ret = eval_cmdlist(e, &node->ngrp);
            break;*/

    case N_PIPELINE:
      ret = eval_pipeline(e, &node->npipe);
      break;

    case N_AND:
    case N_OR:
    case N_NOT:
      ret = eval_and_or(e, &node->nandor);
      break;

    default:
      break;
    }

    if(!list) break;

    node = node->list.next;
  }

  e->exitcode = ret;
  e->flags = oldflags;

  if(ex)
    sh_exit(ret);

  return ret;
}

