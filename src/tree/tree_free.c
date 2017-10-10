#include "shell.h"
#include "tree.h"

/* free a tree node
 * ----------------------------------------------------------------------- */
void tree_free(union node *node)
{
  union node *next;
        
  do
  {
    next = node->list.next;
        
    switch(node->id)
    {
      case N_SIMPLECMD:
        if(node->ncmd.rdir) tree_free(node->ncmd.rdir);
        if(node->ncmd.args) tree_free(node->ncmd.args);
        if(node->ncmd.vars) tree_free(node->ncmd.vars);
        break;
      case N_PIPELINE:
        if(node->npipe.cmds) tree_free(node->npipe.cmds);
        break;
      case N_AND:
      case N_OR:
      case N_NOT:
        if(node->nandor.cmd0) tree_free(node->nandor.cmd0);
        if(node->nandor.cmd1) tree_free(node->nandor.cmd1);
        break;
      case N_SUBSHELL:
      case N_CMDLIST:
        if(node->ngrp.cmds) tree_free(node->ngrp.cmds);
        break;
      case N_FOR:
        if(node->nfor.cmds) tree_free(node->nfor.cmds);
        if(node->nfor.args) tree_free(node->nfor.args);
        if(node->nfor.varn) shell_free(node->nfor.varn);
        break;
      case N_CASE:
        if(node->ncase.list) tree_free(node->ncase.list);
        if(node->ncase.word) tree_free(node->ncase.word);
        break;
      case N_CASENODE:
        if(node->ncasenode.pats) tree_free(node->ncasenode.pats);
        if(node->ncasenode.cmds) tree_free(node->ncasenode.cmds);
        break;
      case N_IF:
        if(node->nif.cmd0) tree_free(node->nif.cmd0);
        if(node->nif.cmd1) tree_free(node->nif.cmd1);
        if(node->nif.test) tree_free(node->nif.test);
        break;
      case N_WHILE:
      case N_UNTIL:
        if(node->nloop.cmds) tree_free(node->nloop.cmds);
        if(node->nloop.test) tree_free(node->nloop.test);
        break;


      case N_ARG:
        if(node->narg.list) tree_free(node->narg.list);
        if(node->narg.stra.s) shell_free(node->narg.stra.s);
        break;
      case N_REDIR:
        if(node->nredir.list) tree_free(node->nredir.list);
        break;
      case N_ASSIGN:
        if(node->nassign.list) tree_free(node->nassign.list);
        if(node->nassign.stra.s) shell_free(&node->nassign.stra.s);
        break;

      case N_ARGSTR:
        if(node->nargstr.stra.s) shell_free(node->nargstr.stra.s);
        break;
      case N_ARGPARAM:
        if(node->nargparam.name) shell_free(node->nargparam.name);
        if(node->nargparam.word) tree_free(node->nargparam.word);
        break;
      case N_ARGCMD:
        if(node->nargcmd.list) tree_free(node->nargcmd.list);
        break;
      case N_ARGEXPR:
        if(node->nargexpr.left) tree_free(node->nargexpr.left);
        if(node->nargexpr.right) tree_free(node->nargexpr.right);
        break;

      case N_FUNCTION:
        if(node->nfunc.name) shell_free(node->nfunc.name);
        if(node->nfunc.cmds) tree_free(node->nfunc.cmds);
        break;
    }
    
    shell_free(node);

  } while((node = next));
}

