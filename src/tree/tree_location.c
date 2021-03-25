#include "../tree.h"

/* get position of N_ARG nodes
 * ----------------------------------------------------------------------- */
int
tree_location(union node* node, struct location* loc) {
  while(node) {
    switch(node->id) {
      case N_ARGSTR:
        *loc = node->nargstr.loc;
        return 1;
        break;
      case N_ARGPARAM:
        *loc = node->nargparam.loc;
        return 1;
        break;
      case N_ARGCMD:
        if(tree_location(node->nargcmd.list, loc))
          return 1;
        break;
      case N_ARG:
        if(tree_location(node->narg.list, loc))
          return 1;
        break;
      case N_CASE:
        if(tree_location(node->ncase.word, loc))
          return 1;
        if(tree_location(node->ncase.list, loc))
          return 1;
        break;

      case N_SIMPLECMD:
        if(tree_location(node->ncmd.vars, loc))
          return 1;
        if(tree_location(node->ncmd.args, loc))
          return 1;
        if(tree_location(node->ncmd.rdir, loc))
          return 1;
        break;
      default: break;
    }
    switch(node->id) {
      case N_SIMPLECMD:
      case N_SUBSHELL:
      case N_CMDLIST:
      case N_FOR:
      case N_CASE:
      case N_IF:
      case N_WHILE:
      case N_UNTIL:
        if(tree_location(node->ngrp.rdir, loc))
          return 1;
        break;
      default: break;
    }
    switch(node->id) {
      case N_PIPELINE:
      case N_CMDLIST:
      case N_SUBSHELL:
      case N_FOR:
      case N_CASENODE:
        if(tree_location(node->npipe.cmds, loc))
          return 1;
        break;
      default: break;
    }

    node = node->next;
  }
  return 0;
}
