#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../tree.h"

static char*
copy_str(const char* s) {
  return s ? str_dup(s) : NULL;
}

/* deep-copy a node chain (following ->next, like tree_free walks it)
 *
 * used by eval_function to give a function definition its own private
 * copy of the body instead of stealing (moving, then NULLing) the
 * pointers straight out of the defining AST node -- stealing only
 * works if that AST node is evaluated exactly once before its enclosing
 * statement is freed (sh_loop.c frees each top-level statement right
 * after running it), which breaks the moment the same node is
 * evaluated again, e.g. a function defined inside a loop or a
 * repeatedly-invoked subshell: the second visit finds name/body
 * already NULLed out and crashes. Mirrors tree_free.c's per-kind
 * switch field-for-field: any pointer tree_free() recurses into here
 * gets recursively copied, any string tree_free() alloc_free()s here
 * gets str_dup()'d, everything else is carried over by the initial
 * byte_copy() (which also makes shared/unowned fields, like
 * nredir.data, aliased in the copy exactly as tree_free leaves them
 * aliased in the original -- consistent with it never freeing them).
 * ----------------------------------------------------------------------- */
union node*
tree_copy(union node* node) {
  union node* head = NULL;
  union node** nptr = &head;

  for(; node; node = node->next) {
    union node* copy = tree_newnode(node->id);

    byte_copy(copy, tree_nodesizes[node->id], node);
    copy->next = NULL;

    switch(node->id) {
      case N_SIMPLECMD:
        copy->ncmd.rdir = node->ncmd.rdir ? tree_copy(node->ncmd.rdir) : NULL;
        copy->ncmd.args = node->ncmd.args ? tree_copy(node->ncmd.args) : NULL;
        copy->ncmd.vars = node->ncmd.vars ? tree_copy(node->ncmd.vars) : NULL;
        break;

      case N_PIPELINE:
        copy->npipe.cmds = node->npipe.cmds ? tree_copy(node->npipe.cmds) : NULL;
        break;

      case N_AND:
      case N_OR:
      case N_NOT:
        copy->nandor.left = node->nandor.left ? tree_copy(node->nandor.left) : NULL;

        if(node->id != N_NOT)
          copy->nandor.right = node->nandor.right ? tree_copy(node->nandor.right) : NULL;
        break;

      case N_LIST: copy->nlist.cmds = tree_copy(node->nlist.cmds); break;

      case N_SUBSHELL:
      case N_BRACEGROUP:
        copy->ngrp.cmds = node->ngrp.cmds ? tree_copy(node->ngrp.cmds) : NULL;
        break;

      case N_FOR:
        copy->nfor.cmds = node->nfor.cmds ? tree_copy(node->nfor.cmds) : NULL;
        copy->nfor.args = node->nfor.args ? tree_copy(node->nfor.args) : NULL;
        copy->nfor.varn = copy_str(node->nfor.varn);
        break;

      case N_CASE:
        copy->ncase.list = node->ncase.list ? tree_copy(node->ncase.list) : NULL;
        copy->ncase.word = node->ncase.word ? tree_copy(node->ncase.word) : NULL;
        break;

      case N_CASENODE:
        copy->ncasenode.pats = node->ncasenode.pats ? tree_copy(node->ncasenode.pats) : NULL;
        copy->ncasenode.cmds = node->ncasenode.cmds ? tree_copy(node->ncasenode.cmds) : NULL;
        break;

      case N_IF:
        copy->nif.cmd0 = node->nif.cmd0 ? tree_copy(node->nif.cmd0) : NULL;
        copy->nif.cmd1 = node->nif.cmd1 ? tree_copy(node->nif.cmd1) : NULL;
        copy->nif.test = node->nif.test ? tree_copy(node->nif.test) : NULL;
        break;

      case N_WHILE:
      case N_UNTIL:
        copy->nloop.cmds = node->nloop.cmds ? tree_copy(node->nloop.cmds) : NULL;
        copy->nloop.test = node->nloop.test ? tree_copy(node->nloop.test) : NULL;
        break;

      case N_ARG:
      case N_ASSIGN:
        copy->narg.list = node->narg.list ? tree_copy(node->narg.list) : NULL;
        /* the initial byte_copy() above aliased copy->narg.stra.s onto
           node->narg.stra.s; without zeroing it first, stralloc_ready()
           (called by stralloc_copy() below) sees a buffer that already
           looks big enough and reuses it in place instead of allocating
           a fresh one, leaving both copies pointing at the same buffer
           -> double free once either side is torn down. */
        byte_zero(&copy->narg.stra, sizeof(copy->narg.stra));

        if(node->narg.stra.s)
          stralloc_copy(&copy->narg.stra, &node->narg.stra);
        break;

      case N_REDIR:
        copy->nredir.word = node->nredir.word ? tree_copy(node->nredir.word) : NULL;
        break;

      case N_ARGSTR:
        byte_zero(&copy->nargstr.stra, sizeof(copy->nargstr.stra));

        if(node->nargstr.stra.s)
          stralloc_copy(&copy->nargstr.stra, &node->nargstr.stra);
        break;

      case N_ARGPARAM:
        copy->nargparam.name = copy_str(node->nargparam.name);
        copy->nargparam.word = node->nargparam.word ? tree_copy(node->nargparam.word) : NULL;
        break;

      case N_ARGCMD:
        copy->nargcmd.list = node->nargcmd.list ? tree_copy(node->nargcmd.list) : NULL;
        break;

      case N_ARGARITH:
        copy->nargarith.tree = node->nargarith.tree ? tree_copy(node->nargarith.tree) : NULL;
        break;

      case A_NUM: break;

      case A_TERNARY:
        copy->narithternary.cond =
            node->narithternary.cond ? tree_copy(node->narithternary.cond) : NULL;
        copy->narithternary.ontrue =
            node->narithternary.ontrue ? tree_copy(node->narithternary.ontrue) : NULL;
        copy->narithternary.onfalse =
            node->narithternary.onfalse ? tree_copy(node->narithternary.onfalse) : NULL;
        break;

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
        copy->narithbinary.left =
            node->narithbinary.left ? tree_copy(node->narithbinary.left) : NULL;
        copy->narithbinary.right =
            node->narithbinary.right ? tree_copy(node->narithbinary.right) : NULL;
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
        copy->narithunary.node = node->narithunary.node ? tree_copy(node->narithunary.node) : NULL;
        break;

      case N_FUNCTION:
        copy->nfunc.name = copy_str(node->nfunc.name);
        copy->nfunc.body = node->nfunc.body ? tree_copy(node->nfunc.body) : NULL;
        break;
    }

    tree_link(copy, nptr);
  }

  return head;
}
