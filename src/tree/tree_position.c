#include "../tree.h"

/* get position of N_ARG nodes
 * ----------------------------------------------------------------------- */
int
tree_position(union node* node, struct position* pos) {
  while(node) {
    switch(node->id) {
      case N_ARGSTR:
        *pos = node->nargstr.pos;
        return 1;
        break;
      case N_ARGPARAM:
        *pos = node->nargparam.pos;
        return 1;
        break;
      case N_ARGCMD:
        if(tree_position(node->nargcmd.list, pos))
          return 1;
        break;
      case N_ARG:
        if(tree_position(node->narg.list, pos))
          return 1;
        break;
      case N_CASE:
        if(tree_position(node->ncase.word, pos))
          return 1;
        if(tree_position(node->ncase.list, pos))
          return 1;
        break;

      case N_SIMPLECMD:
        if(tree_position(node->ncmd.vars, pos))
          return 1;
        if(tree_position(node->ncmd.args, pos))
          return 1;
        if(tree_position(node->ncmd.rdir, pos))
          return 1;
        break;
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
        if(tree_position(node->ngrp.rdir, pos))
          return 1;
        break;
    }
    switch(node->id) {
      case N_PIPELINE:
      case N_CMDLIST:
      case N_SUBSHELL:
      case N_FOR:
      case N_CASENODE:
        if(tree_position(node->npipe.cmds, pos))
          return 1;
        break;
    }

    node = node->next;
  }
  return 0;
}
