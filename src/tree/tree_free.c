#include "../../lib/alloc.h"
#include "../tree.h"

/* free a tree node
 * ----------------------------------------------------------------------- */
void
tree_free(union node* node) {
  union node* next;

  do {
    next = node->next;

    switch(node->id) {
      case N_SIMPLECMD:
        if(node->ncmd.rdir)
          tree_free(node->ncmd.rdir);

        if(node->ncmd.args)
          tree_free(node->ncmd.args);

        if(node->ncmd.vars)
          tree_free(node->ncmd.vars);
        break;

      case N_PIPELINE:
        if(node->npipe.cmds)
          tree_free(node->npipe.cmds);
        break;

      case N_AND:
      case N_OR:
      case N_NOT:
        if(node->nandor.left)
          tree_free(node->nandor.left);

        if(node->nandor.right)
          tree_free(node->nandor.right);
        break;

      case N_LIST: tree_free(node->nlist.cmds); break;

      case N_SUBSHELL:
      case N_BRACEGROUP:
        if(node->ngrp.cmds)
          tree_free(node->ngrp.cmds);
        break;

      case N_FOR:
        if(node->nfor.cmds)
          tree_free(node->nfor.cmds);

        if(node->nfor.args)
          tree_free(node->nfor.args);

        if(node->nfor.varn)
          alloc_free(node->nfor.varn);
        break;

      case N_CASE:
        if(node->ncase.list)
          tree_free(node->ncase.list);

        if(node->ncase.word)
          tree_free(node->ncase.word);
        break;

      case N_CASENODE:
        if(node->ncasenode.pats)
          tree_free(node->ncasenode.pats);

        if(node->ncasenode.cmds)
          tree_free(node->ncasenode.cmds);
        break;

      case N_IF:
        if(node->nif.cmd0)
          tree_free(node->nif.cmd0);

        if(node->nif.cmd1)
          tree_free(node->nif.cmd1);

        if(node->nif.test)
          tree_free(node->nif.test);
        break;

      case N_WHILE:
      case N_UNTIL:
        if(node->nloop.cmds)
          tree_free(node->nloop.cmds);

        if(node->nloop.test)
          tree_free(node->nloop.test);
        break;

      case N_ARG:
      case N_ASSIGN:
        if(node->narg.list)
          tree_free(node->narg.list);
        stralloc_free(&node->narg.stra);
        break;

      case N_REDIR:
        if(node->nredir.word)
          tree_free(node->nredir.word);
        break;

      case N_ARGSTR:
        if(node->nargstr.stra.s)
          stralloc_free(&node->nargstr.stra);
        break;

      case N_ARGPARAM:
        if(node->nargparam.name)
          alloc_free(node->nargparam.name);

        if(node->nargparam.word)
          tree_free(node->nargparam.word);
        break;

      case N_ARGCMD:
        if(node->nargcmd.list)
          tree_free(node->nargcmd.list);
        break;

      case N_ARGARITH:
        if(node->nargarith.tree)
          tree_free(node->nargarith.tree);
        break;

      case A_NUM:
        break;

        //    case A_VAR: alloc_free(node->narithvar.var); break;

      case A_OR:
      case A_AND:
      case A_BITOR:
      case A_BITXOR:
      case A_BITAND:
      case A_EQ:
      case A_NE:
      case A_LT:
      case A_GT:
      case A_GE:
      case A_LE:
      case A_SHL:
      case A_SHR:
      case A_ADD:
      case A_SUB:
      case A_MUL:
      case A_DIV:
      case A_MOD:
      case A_EXP:
        if(node->narithbinary.left)
          tree_free(node->narithbinary.left);

        if(node->narithbinary.right)
          tree_free(node->narithbinary.right);
        break;

      case A_PAREN:
      case A_UNARYMINUS:
      case A_UNARYPLUS:
      case A_NOT:
      case A_BNOT:
      case A_PREINCR:
      case A_PREDECR:
      case A_POSTINCR:
      case A_POSTDECR:
        if(node->narithunary.node)
          tree_free(node->narithunary.node);
        break;

      case A_VASSIGN:
      case A_VADD:
      case A_VSUB:
      case A_VMUL:
      case A_VDIV:
      case A_VMOD:
      case A_VSHL:
      case A_VSHR:
      case A_VBITAND:
      case A_VBITXOR:
      case A_VBITOR:
        tree_free(node->narithbinary.left);
        tree_free(node->narithbinary.right);
        break;

      case N_FUNCTION:
        if(node->nfunc.name)
          alloc_free(node->nfunc.name);

        if(node->nfunc.body)
          tree_free(node->nfunc.body);
        break;
    }

    alloc_free(node);

  } while((node = next));
}
