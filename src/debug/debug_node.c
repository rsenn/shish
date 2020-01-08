#include "debug.h"

#ifdef DEBUG
#include "tree.h"

/* debugs a tree node!
 * ----------------------------------------------------------------------- */
const char* debug_nodes[] = {
    "N_SIMPLECMD",
    "N_PIPELINE",
    "N_AND",
    "N_OR",
    "N_NOT",
    "N_SUBSHELL",
    "N_CMDLIST",
    "N_FOR",
    "N_CASE",
    "N_CASENODE",
    "N_IF",
    "N_WHILE",
    "N_UNTIL",
    "N_FUNCTION",
    "N_ARG",
    "N_ASSIGN",
    "N_REDIR",
    "N_ARGSTR",
    "N_ARGCMD",
    "N_ARGPARAM",
    "N_ARGARITH",
    "N_ARITH_NUM",
    "N_ARITH_PAREN",
    "N_ARITH_OR",
    "N_ARITH_AND",
    "N_ARITH_BOR",
    "N_ARITH_BXOR",
    "N_ARITH_BAND",
    "N_ARITH_EQ",
    "N_ARITH_NE",
    "N_ARITH_LT",
    "N_ARITH_GT",
    "N_ARITH_GE",
    "N_ARITH_LE",
    "N_ARITH_LSHIFT",
    "N_ARITH_RSHIFT",
    "N_ARITH_ADD",
    "N_ARITH_SUB",
    "N_ARITH_MUL",
    "N_ARITH_DIV",
    "N_ARITH_MOD",
    "N_ARITH_EXP",
    "N_ARITH_UNARYMINUS",
    "N_ARITH_UNARYPLUS",
    "N_ARITH_NOT",
    "N_ARITH_BNOT",
};

void
debug_node(union node* node, int depth) {
  debug_unquoted(NULL, debug_nodes[node->id], depth);
  debug_space(depth, node->id != N_ARITH_NUM ? 1 : 0);

  switch(node->id) {
    case N_SIMPLECMD:
      debug_ulong("bngd", node->ncmd.bgnd, depth);
      debug_space(depth, 1);
      debug_sublist("rdir", node->ncmd.rdir, depth);
      debug_space(depth, 1);
      debug_sublist("args", node->ncmd.args, depth);
      debug_space(depth, 1);
      debug_sublist("vars", node->ncmd.vars, depth);
      break;

    case N_PIPELINE:
      debug_ulong("bgnd", node->npipe.bgnd, depth);
      debug_space(depth, 1);
      debug_sublist("cmds", node->npipe.cmds, depth);
      debug_space(depth, 1);
      debug_ulong("ncmd", node->npipe.ncmd, depth);
      break;

    case N_AND:
    case N_OR:
      debug_ulong("bgnd", node->nandor.bgnd, depth);
      debug_space(depth, 1);
      debug_subnode("cmd0", node->nandor.cmd0, depth);
      debug_space(depth, 1);
      debug_subnode("cmd1", node->nandor.cmd1, depth);
      break;

    case N_SUBSHELL:
    case N_CMDLIST:
      debug_sublist("rdir", node->ngrp.rdir, depth);
      debug_space(depth, 1);
      debug_sublist("cmds", node->ngrp.cmds, depth);
      break;

    case N_FOR:
      debug_str("varn", node->nfor.varn, depth);
      debug_space(depth, 1);
      debug_sublist("cmds", node->nfor.cmds, depth);
      debug_space(depth, 1);
      debug_sublist("args", node->nfor.args, depth);
      break;

    case N_CASE:
      debug_ulong("bgnd", node->ncase.bgnd, depth);
      debug_space(depth, 1);
      debug_sublist("rdir", node->ncase.rdir, depth);
      debug_space(depth, 1);
      debug_sublist("list", node->ncase.list, depth);
      debug_space(depth, 1);
      debug_sublist("word", node->ncase.word, depth);
      break;

    case N_CASENODE:
      debug_sublist("pats", node->ncasenode.pats, depth);
      debug_space(depth, 1);
      debug_sublist("cmds", node->ncasenode.cmds, depth);
      break;

    case N_IF:
      debug_ulong("bgnd", node->nif.bgnd, depth);
      debug_space(depth, 1);
      debug_sublist("rdir", node->nif.rdir, depth);
      debug_space(depth, 1);
      debug_sublist("cmd0", node->nif.cmd0, depth);
      debug_space(depth, 1);
      debug_sublist("cmd1", node->nif.cmd1, depth);
      debug_space(depth, 1);
      debug_subnode("test", node->nif.test, depth);
      break;

    case N_WHILE:
    case N_UNTIL:
      debug_ulong("bgnd", node->nloop.bgnd, depth);
      debug_space(depth, 1);
      debug_sublist("rdir", node->nif.rdir, depth);
      debug_space(depth, 1);
      debug_sublist("cmds", node->nloop.test, depth);
      debug_space(depth, 1);
      debug_subnode("test", node->nloop.test, depth);
      break;

    case N_FUNCTION:
      debug_sublist("cmds", node->nfunc.cmds, depth);
      debug_space(depth, 1);
      debug_str("name", node->nfunc.name, depth);
      break;

    case N_ASSIGN:

    case N_ARG:
      debug_subst("flag", node->narg.flag, depth);
      debug_space(depth, 1);
      debug_stralloc("stra", &node->narg.stra, depth);
      debug_space(depth, 1);
      debug_sublist("list", node->narg.list, depth);
      /*      debug_space(depth, 1);
            debug_sublist("next", node->narg.next, depth);
      */
      break;

    case N_REDIR:
      debug_redir("flag", node->nredir.flag, depth);
      debug_space(depth, 1);
      debug_sublist("list", node->nredir.list, depth);
      debug_space(depth, 1);
      debug_sublist("data", node->nredir.data, depth);
      debug_space(depth, 1);
      debug_ulong("fdes", node->nredir.fdes, depth);
      break;

    case N_ARGSTR:
      debug_subst("flag", node->nargstr.flag, depth);
      debug_space(depth, 1);
      debug_stralloc("stra", &node->nargstr.stra, depth);
      break;

    case N_ARGPARAM:
      debug_subst("flag", node->nargparam.flag, depth);
      debug_space(depth, 1);
      debug_str("name", node->nargparam.name, depth);
      debug_space(depth, 1);
      debug_sublist("word", node->nargparam.word, depth);
      debug_space(depth, 1);
      debug_ulong("numb", node->nargparam.numb, depth);
      break;

    case N_ARGCMD:
    case N_ARGARITH:
      /*   debug_subst("flag", node->nargcmd.flag, depth);
         debug_space(depth, 1);*/
      debug_sublist("tree", node->nargarith.tree, depth);
      break;

    case N_ARITH_NUM:
      debug_ulong(0, node->narithnum.num, depth);
      break;

      //    case N_ARITH_VAR: debug_str("var", node->narithvar.var, depth); break;

    case N_ARITH_ADD:
    case N_ARITH_SUB:
    case N_ARITH_MUL:
    case N_ARITH_DIV:
    case N_ARITH_OR:
    case N_ARITH_AND:
    case N_ARITH_BOR:
    case N_ARITH_BXOR:
    case N_ARITH_BAND:
    case N_ARITH_EQ:
    case N_ARITH_NE:
    case N_ARITH_LT:
    case N_ARITH_GT:
    case N_ARITH_GE:
    case N_ARITH_LE:
    case N_ARITH_LSHIFT:
    case N_ARITH_RSHIFT:
    case N_ARITH_MOD:
    case N_ARITH_EXP:
      debug_sublist("left", node->narithbinary.left, depth);
      debug_space(depth, 1);
      debug_sublist("right", node->narithbinary.right, depth);
      break;

    case N_ARITH_PAREN: debug_sublist("tree", node->nargarith.tree, depth); break;

    case N_ARITH_NOT:
    case N_ARITH_BNOT:
    case N_ARITH_UNARYMINUS:
    case N_ARITH_UNARYPLUS: debug_sublist("node", node->narithunary.node, depth); break;

    case N_NOT: debug_sublist("cmds", node->nandor.cmd0, depth); break;
  }
}
#endif /* DEBUG */
