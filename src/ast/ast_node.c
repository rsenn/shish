#include "../ast.h"
#include "../expand.h"
#include "../tree.h"
#include "../../lib/str.h"

/* one node as a JSON object; its children follow through ast_kids()/ast_kid()
 * ----------------------------------------------------------------------- */
void
ast_node(struct ast* a, union node* n) {
  struct json* j = &a->j;

  json_open(j, '{');
  json_kstr(j, "kind", ast_names[n->id]);

  switch(n->id) {
    case N_SIMPLECMD:
      ast_bgnd(a, n->ncmd.bgnd);

      if(n->ncmd.vars)
        ast_kids(a, "vars", n->ncmd.vars);

      if(n->ncmd.args)
        ast_kids(a, "args", n->ncmd.args);

      ast_rdir(a, n->ncmd.rdir);
      break;

    case N_PIPELINE:
      ast_bgnd(a, n->npipe.bgnd);
      ast_kids(a, "cmds", n->npipe.cmds);
      json_kuint(j, "ncmd", n->npipe.ncmd);
      break;

    case N_AND:
    case N_OR:
      ast_bgnd(a, n->nandor.bgnd);
      ast_kid(a, "left", n->nandor.left);
      ast_kid(a, "right", n->nandor.right);
      break;

    case N_SUBSHELL:
    case N_BRACEGROUP:
      ast_kids(a, "cmds", n->ngrp.cmds);
      ast_rdir(a, n->ngrp.rdir);
      break;

    case N_FOR:
      json_kstr(j, "varn", n->nfor.varn);
      ast_pos(a, &n->nfor.loc, str_len(n->nfor.varn));
      ast_kids(a, "cmds", n->nfor.cmds);
      ast_kids(a, "args", n->nfor.args);
      break;

    case N_CASE:
      ast_bgnd(a, n->ncase.bgnd);
      ast_rdir(a, n->ncase.rdir);
      ast_kids(a, "list", n->ncase.list);
      ast_kids(a, "word", n->ncase.word);
      break;

    case N_CASENODE:
      ast_kids(a, "pats", n->ncasenode.pats);
      ast_kids(a, "cmds", n->ncasenode.cmds);
      break;

    case N_IF:
      ast_bgnd(a, n->nif.bgnd);
      ast_rdir(a, n->nif.rdir);
      ast_kids(a, "cmd0", n->nif.cmd0);

      if(n->nif.cmd1)
        ast_kids(a, "cmd1", n->nif.cmd1);

      ast_kid(a, "test", n->nif.test);
      break;

    case N_WHILE:
    case N_UNTIL:
      ast_bgnd(a, n->nloop.bgnd);
      ast_rdir(a, n->nloop.rdir);
      ast_kid(a, "test", n->nloop.test);
      ast_kids(a, "cmds", n->nloop.cmds);
      break;

    case N_FUNCTION:
      json_kstr(j, "name", n->nfunc.name);
      ast_pos(a, &n->nfunc.loc, str_len(n->nfunc.name));
      ast_kids(a, "body", n->nfunc.body);
      break;

    case N_ASSIGN:
    case N_ARG:
      if(n->narg.flag)
        json_khex(j, "flag", n->narg.flag);

      if(n->narg.list)
        ast_kids(a, "list", n->narg.list);

      break;

    case N_REDIR:
      json_kuint(j, "flag", n->nredir.flag);
      ast_kids(a, "word", n->nredir.word);
      json_kuint(j, "fdes", n->nredir.fdes);
      break;

    case N_ARGSTR:
      json_khex(j, "flag", n->nargstr.flag);
      ast_pos(a, &n->nargstr.loc, n->nargstr.len);
      json_key(j, "stra");
      json_str(j, n->nargstr.s, n->nargstr.len);
      break;

    case N_ARGPARAM:
      json_khex(j, "flag", n->nargparam.flag);
      json_kstr(j, "name", n->nargparam.name);

      if(n->nargparam.word)
        ast_kids(a, "word", n->nargparam.word);
      else {
        json_key(j, "word");
        json_null(j);
      }

      if((n->nargparam.flag & S_SPECIAL) == S_ARG)
        json_kuint(j, "numb", n->nargparam.numb);

      ast_pos(a, &n->nargparam.loc, str_len(n->nargparam.name));
      break;

    case N_ARGCMD:
      json_khex(j, "flag", n->nargcmd.flag);
      ast_kids(a, "list", n->nargcmd.list);
      break;

    case N_ARGARITH:
      json_khex(j, "flag", n->nargarith.flag);
      ast_kids(a, "tree", n->nargarith.tree);
      break;

    case A_NUM:
      json_kuint(j, "num", n->narithnum.num);
      json_kuint(j, "base", n->narithnum.base);
      break;

    case A_ADD:
    case A_SUB:
    case A_MUL:
    case A_DIV:
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
      ast_kid(a, "left", n->narithbinary.left);
      ast_kid(a, "right", n->narithbinary.right);
      break;

    case A_PAREN: ast_kid(a, "tree", n->nargarith.tree); break;

    case A_TERNARY:
      ast_kid(a, "cond", n->narithternary.cond);
      ast_kid(a, "ontrue", n->narithternary.ontrue);
      ast_kid(a, "onfalse", n->narithternary.onfalse);
      break;

    case A_NOT:
    case A_BNOT:
    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_PREINCR:
    case A_PREDECR:
    case A_POSTINCR:
    case A_POSTDECR: ast_kid(a, "node", n->narithunary.node); break;

    case N_NOT: ast_kids(a, "cmds", n->nnot.pipeline); break;
    case N_TIME: ast_kids(a, "cmds", n->ntime.pipeline); break;
    case N_LIST: ast_kids(a, "cmds", n->nlist.cmds); break;
  }

  json_close(j, '}');
}
