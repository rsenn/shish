#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../expand.h"
#include "../../lib/str.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../tree.h"
#include "../fd.h"

int debug_nindent = 2;
extern int sh_no_position;

/* debugs a tree node!
 * ----------------------------------------------------------------------- */
const char* debug_nodes[] = {"N_SIMPLECMD",
                             "N_PIPELINE",
                             "N_AND",
                             "N_OR",
                             "N_NOT",
                             "N_SUBSHELL",
                             "N_BRACEGROUP",
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
                             "N_ARGRANGE",
                             "A_NUM",
                             "A_PAREN",
                             "A_OR",
                             "A_AND",
                             "A_BOR",
                             "A_BXOR",
                             "A_BAND",
                             "A_EQ",
                             "A_NE",
                             "A_LT",
                             "A_GT",
                             "A_GE",
                             "A_LE",
                             "A_LSHIFT",
                             "A_RSHIFT",
                             "A_ADD",
                             "A_SUB",
                             "A_MUL",
                             "A_DIV",
                             "A_MOD",
                             "A_EXP",
                             "A_UNARYMINUS",
                             "A_UNARYPLUS",
                             "A_NOT",
                             "A_BNOT",
                             "A_PREDECREMENT",
                             "A_PREINCREMENT",
                             "A_POSTDECREMENT",
                             "A_POSTINCREMENT",
                             "A_ASSIGN",
                             "A_ASSIGNADD",
                             "A_ASSIGNSUB",
                             "A_ASSIGNMUL",
                             "A_ASSIGNDIV",
                             "A_ASSIGNMOD",
                             "A_ASSIGNLSHIFT",
                             "A_ASSIGNRSHIFT",
                             "A_ASSIGNBAND",
                             "A_ASSIGNBXOR",
                             "A_ASSIGNBOR"};

void
debug_node(union node* node, int depth) {
  const char* name;

  name = debug_nodes[node->id];

  // debug_indent(depth);
  debug_c('{');
  debug_str(" kind", name, depth, debug_quote);

  switch(node->id) {
    case N_SIMPLECMD:

      debug_ulong(", bngd", node->ncmd.bgnd, depth);

      // if(node->ncmd.vars)
      debug_sublist(", vars", node->ncmd.vars, depth);

      // if(node->ncmd.args)
      debug_sublist(", args", node->ncmd.args, depth);

      // if(node->ncmd.rdir)
      debug_sublist(", rdir", node->ncmd.rdir, depth);

      break;
    case N_PIPELINE:

      debug_ulong(", bgnd", node->npipe.bgnd, depth);
      debug_sublist(", cmds", node->npipe.cmds, depth);
      debug_ulong(", ncmd", node->npipe.ncmd, depth);
      break;

    case N_AND:
    case N_OR:

      debug_ulong(", bgnd", node->nandor.bgnd, depth);

      debug_subnode(", left", node->nandor.left, depth);
      if(node->nandor.right)
        debug_subnode(", right", node->nandor.right, depth);
      break;

    case N_SUBSHELL:
    case N_BRACEGROUP:

      debug_sublist(", cmds", node->ngrp.cmds, depth);

      if(node->ngrp.rdir) {
        debug_sublist(", rdir", node->ngrp.rdir, depth);
      }

      break;

    case N_FOR:
      debug_str(", varn", node->nfor.varn, depth, debug_quote);
      debug_sublist(", cmds", node->nfor.cmds, depth);
      debug_sublist(", args", node->nfor.args, depth);
      break;

    case N_CASE:

      debug_ulong(", bgnd", node->ncase.bgnd, depth);
      debug_sublist(", rdir", node->ncase.rdir, depth);
      debug_sublist(", list", node->ncase.list, depth);
      debug_sublist(", word", node->ncase.word, depth);
      break;

    case N_CASENODE:
      debug_sublist(", pats", node->ncasenode.pats, depth);
      debug_sublist(", cmds", node->ncasenode.cmds, depth);
      break;

    case N_IF:

      debug_ulong(", bgnd", node->nif.bgnd, depth);
      debug_sublist(", rdir", node->nif.rdir, depth);
      debug_sublist(", cmd0", node->nif.cmd0, depth);
      if(node->nif.cmd1) {
        debug_sublist(", cmd1", node->nif.cmd1, depth);
      }
      debug_subnode(", test", node->nif.test, depth);
      break;

    case N_WHILE:
    case N_UNTIL:
      debug_ulong(", bgnd", node->nloop.bgnd, depth);
      debug_sublist(", rdir", node->nif.rdir, depth);
      debug_subnode(", test", node->nloop.test, depth);
      debug_sublist(", cmds", node->nloop.cmds, depth);
      break;

    case N_FUNCTION:
      debug_str(", name", node->nfunc.name, depth, debug_quote);

      debug_position(", loc", &node->nfunc.loc, depth);
      debug_sublist(", body", node->nfunc.body, depth);
      break;

    case N_ASSIGN:
    case N_ARG:
      debug_ulong(", flag", node->narg.flag, depth);
      //      debug_subst(0, node->narg.flag, -1);

      if(node->narg.stra.len > 0)
        debug_stralloc(", stra", &node->narg.stra, depth, debug_quote);

      if(node->narg.list)
        debug_sublist(", list", node->narg.list, depth);
      /*      debug_space(depth, 0);
            debug_sublist(", next", node->next, depth+1);
      */

      break;

    case N_REDIR:
      debug_redir(", flag", node->nredir.flag, depth);
      debug_sublist(", list", node->nredir.list, depth);
      debug_ulong(", fdes", node->nredir.fdes, depth);
      break;

    case N_ARGSTR:

      debug_ulong(", flag", node->nargstr.flag /*& 0x7*/, depth);
      if(!sh_no_position)
        debug_position(", loc", &node->nargstr.loc, depth); // node->nargstr.flag & S_DQUOTED ? '"' : node->nargstr.flag & S_SQUOTED ? '\'' : '\0');
      debug_stralloc(", stra",
                     &node->nargstr.stra,
                     depth,
                     debug_quote); // node->nargstr.flag & S_DQUOTED ? '"' : node->nargstr.flag & S_SQUOTED ? '\'' : '\0');
      break;

    case N_ARGPARAM: {
      /*   int flag = (node->nargstr.flag & 0x7);
         debug_subst(0, flag, depth+1);
         if(flag)
           buffer_putspace(&debug_buffer);

         debug_s("${");
         debug_str(0, node->nargparam.name, depth, 0);
         debug_s("}");

         if((node->nargparam.flag & S_VAR) >> 8) {
           debug_sublist(", word", node->nargparam.word, -1);
         }
         if(node->nargparam.numb > 0) {
           debug_ulong(",  numb", node->nargparam.numb, depth+1);
         }*/

      debug_ulong(", flag", node->nargparam.flag, depth);
      if(!sh_no_position)
        debug_position(", loc", &node->nargparam.loc, depth);
      debug_str(", name", node->nargparam.name, depth, debug_quote);
      debug_sublist(", word", node->nargparam.word, depth);
      debug_ulong(", numb", node->nargparam.numb, depth);

      break;
    }
    case N_ARGCMD:
      debug_ulong(", flag", node->nargcmd.flag, depth);
      debug_sublist(", list", node->nargcmd.list, depth);
      break;
    case N_ARGARITH:
      /*   debug_subst(0,node->nargcmd.flag, depth+1);
         //debug_space(depth, 0);*/
      debug_ulong(", flag", node->nargarith.flag, depth);
      debug_sublist(", tree", node->nargarith.tree, depth);
      break;

    case A_NUM: debug_ulong(", num", node->narithnum.num, depth); break;

    case A_ADD:
    case A_SUB:
    case A_MUL:
    case A_DIV:
    case A_OR:
    case A_AND:
    case A_BOR:
    case A_BXOR:
    case A_BAND:
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_GE:
    case A_LE:
    case A_LSHIFT:
    case A_RSHIFT:
    case A_MOD:
    case A_EXP:
    case A_ASSIGN:
    case A_ASSIGNADD:
    case A_ASSIGNSUB:
    case A_ASSIGNMUL:
    case A_ASSIGNDIV:
    case A_ASSIGNMOD:
    case A_ASSIGNLSHIFT:
    case A_ASSIGNRSHIFT:
    case A_ASSIGNBAND:
    case A_ASSIGNBXOR:
    case A_ASSIGNBOR:
      debug_subnode(", left", node->narithbinary.left, depth);
      debug_subnode(", right", node->narithbinary.right, depth);
      break;

    case A_PAREN: debug_subnode(", tree", node->nargarith.tree, depth); break;

    case A_NOT:
    case A_BNOT:
    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_PREINCREMENT:
    case A_PREDECREMENT:
    case A_POSTINCREMENT:
    case A_POSTDECREMENT: debug_subnode(", node", node->narithunary.node, depth); break;

    case N_NOT: debug_sublist(", cmds", node->nandor.left, depth); break;
  }

  debug_newline(depth - 1);
  debug_c('}');
}
#endif /* DEBUG_OUTPUT */
