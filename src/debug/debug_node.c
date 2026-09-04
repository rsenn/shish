#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../expand.h"
#include "../../lib/str.h"

int debug_nindent = 2;

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../tree.h"
#include "../fd.h"

extern int sh_no_position;

/* JSON "kind" names, one per union node/arithmetic-op enum value (tree.h).
 * Command-tree nodes are named after POSIX Shell Command Language (2.10)
 * grammar rules; arithmetic-expression nodes (which POSIX defers to C's
 * grammar for) get plain descriptive lower_snake_case names instead. */
const char* debug_nodes[] = {
    "simple_command",       "pipeline",       "and",                   "or",
    "not",                  "list",           "subshell",              "brace_group",
    "for_clause",           "case_clause",    "case_item",             "if_clause",
    "while_clause",         "until_clause",   "function_definition",   "word",
    "assignment",           "redirect",       "string",                "command_substitution",
    "parameter_expansion",  "arithmetic_expansion",
    "number",               "paren_expr",     "conditional_expr",      "logical_or",
    "logical_and",          "bitwise_or",     "bitwise_xor",           "bitwise_and",
    "equal",                "not_equal",      "less_than",             "greater_than",
    "greater_equal",        "less_equal",     "shift_left",            "shift_right",
    "add",                  "subtract",       "multiply",              "divide",
    "modulo",               "exponent",       "unary_minus",           "unary_plus",
    "logical_not",          "bitwise_not",    "pre_decrement",         "pre_increment",
    "post_decrement",       "post_increment", "assign",                "add_assign",
    "sub_assign",           "mul_assign",     "div_assign",            "mod_assign",
    "shl_assign",           "shr_assign",     "and_assign",            "xor_assign",
    "or_assign",
};

void
debug_node(union node* node, int depth) {
  const char* name;

  // if(depth > 0 && node->id >= N_ARG)
  // depth = -2;
  name = debug_nodes[node->id];

  // debug_indent(depth);
  debug_c('{');
  debug_str(" kind", name, depth, debug_quote);

  switch(node->id) {
    case N_SIMPLECMD:

      debug_ulong(", bgnd", node->ncmd.bgnd, depth);

      if(node->ncmd.vars)
        debug_sublist(", vars", node->ncmd.vars, depth);

      if(node->ncmd.args)
        debug_sublist(", args", node->ncmd.args, depth);

      if(node->ncmd.rdir)
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

      if(node->ngrp.rdir)
        debug_sublist(", rdir", node->ngrp.rdir, depth);

      break;

    case N_FOR:
      debug_str(", varn", node->nfor.varn, depth, debug_quote);
      debug_sublist(", cmds", node->nfor.cmds, depth);
      debug_sublist(", args", node->nfor.args, depth);
      break;

    case N_CASE:

      debug_ulong(", bgnd", node->ncase.bgnd, depth);

      if(node->ncase.rdir)
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

      if(node->nif.rdir)
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

      if(node->nloop.rdir)
        debug_sublist(", rdir", node->nloop.rdir, depth);
      debug_subnode(", test", node->nloop.test, depth);
      debug_sublist(", cmds", node->nloop.cmds, depth);
      break;

    case N_FUNCTION:
      debug_str(", name", node->nfunc.name, depth, debug_quote);

      if(debug_emit_loc)
        debug_location(", loc", &node->nfunc.loc, depth);
      if(debug_emit_range)
        debug_range(", range", node->nfunc.loc.offset,
                    node->nfunc.loc.offset + str_len(node->nfunc.name), depth);
      debug_sublist(", body", node->nfunc.body, depth);
      break;

    case N_ASSIGN:
    case N_ARG:
      if(node->narg.flag)
        debug_xlong(", flag", node->narg.flag, depth);

      if(node->narg.list)
        debug_sublist(", list", node->narg.list, depth);
      else
        debug_stralloc(", stra", &node->narg.stra, depth, debug_quote);

      break;

    case N_REDIR:
      debug_redir(", flag", node->nredir.flag, depth);
      debug_sublist(", word", node->nredir.word, depth);
      debug_ulong(", fdes", node->nredir.fdes, depth);
      break;

    case N_ARGSTR:

      debug_xlong(", flag", node->nargstr.flag /*& 0x7*/, depth);

      if(!sh_no_position) {
        if(debug_emit_loc)
          debug_location(", loc",
                         &node->nargstr.loc,
                         depth); // node->nargstr.flag & S_DQUOTED ? '"' :
                                 // node->nargstr.flag & S_SQUOTED ? '\'' : '\0');
        if(debug_emit_range)
          debug_range(", range", node->nargstr.loc.offset,
                      node->nargstr.loc.offset + node->nargstr.stra.len, depth);
      }
      debug_stralloc(", stra",
                     &node->nargstr.stra,
                     depth,
                     debug_quote); // node->nargstr.flag & S_DQUOTED ? '"' :
                                   // node->nargstr.flag & S_SQUOTED ? '\'' : '\0');
      break;

    case N_ARGPARAM: {
      /*   int flag = (node->nargstr.flag & 0x7);
         debug_subst(0, flag);

if(flag)
           buffer_putspace(debug_output);

         debug_s("${");
         debug_str(0, node->nargparam.name, depth, 0);
         debug_s("}");

         if((node->nargparam.flag & S_VAR) >> 8) {
           debug_sublist(", word", node->nargparam.word, -1);
         }

if(node->nargparam.numb > 0) {
           debug_ulong(",  numb", node->nargparam.numb, depth+1);
         }*/

      debug_xlong(", flag", node->nargparam.flag, depth);
      debug_str(", name", node->nargparam.name, depth, debug_quote);

      if(node->nargparam.word)
        debug_sublist(", word", node->nargparam.word, depth);
      else
        debug_str(", word", "null", depth, 0);

      if((node->nargparam.flag & S_SPECIAL) == S_ARG)
        debug_ulong(", numb", node->nargparam.numb, depth);

      if(!sh_no_position) {
        if(debug_emit_loc)
          debug_location(", loc", &node->nargparam.loc, depth);
        if(debug_emit_range)
          debug_range(", range", node->nargparam.loc.offset,
                      node->nargparam.loc.offset + str_len(node->nargparam.name), depth);
      }

      break;
    }

    case N_ARGCMD:
      debug_xlong(", flag", node->nargcmd.flag, depth);
      debug_sublist(", list", node->nargcmd.list, depth);
      break;

    case N_ARGARITH:
      /*   debug_subst(0,node->nargcmd.flag);
         //debug_space(depth, 0);*/
      debug_xlong(", flag", node->nargarith.flag, depth);
      debug_sublist(", tree", node->nargarith.tree, depth);
      break;

    case A_NUM:
      debug_ulong(", num", node->narithnum.num, depth);
      debug_ulong(", base", node->narithnum.base, depth);
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
      debug_subnode(", left", node->narithbinary.left, depth);
      debug_subnode(", right", node->narithbinary.right, depth);
      break;

    case A_PAREN: debug_subnode(", tree", node->nargarith.tree, depth); break;

    case A_TERNARY:
      debug_subnode(", cond", node->narithternary.cond, depth);
      debug_subnode(", ontrue", node->narithternary.ontrue, depth);
      debug_subnode(", onfalse", node->narithternary.onfalse, depth);
      break;

    case A_NOT:
    case A_BNOT:
    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_PREINCR:
    case A_PREDECR:
    case A_POSTINCR:
    case A_POSTDECR: debug_subnode(", node", node->narithunary.node, depth); break;

    case N_NOT: debug_sublist(", cmds", node->nandor.left, depth); break;
    case N_LIST: debug_sublist(", cmds", node->nlist.cmds, depth); break;
  }

  debug_newline(depth - 1);
  debug_c('}');
}
#endif /* DEBUG_OUTPUT */
