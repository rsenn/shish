#include "../../lib/uint64.h"
#include "../../lib/fmt.h"
#include "../../lib/byte.h"
#include "../expand.h"
#include "../features.h"
#include "../parse.h"
#include "../redir.h"
#include "../tree.h"
#include <stdlib.h>

void
tree_cat(union node* node, stralloc* sa) {
  tree_cat_n(node, sa, 0);
  stralloc_nul(sa);
}

/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_cat_n(union node* node, stralloc* sa, int depth) {
  const char* sep = NULL;
again:
  switch(node->id) {
    case N_SIMPLECMD: {

      /* concatenate vars */
      if(node->ncmd.vars)
        tree_catlist_n(node->ncmd.vars, sa, " ", depth + 1);

      if(node->ncmd.vars && node->ncmd.args)
        tree_catseparator(sa, " ", depth);

      /* concatenate arguments */
      if(node->ncmd.args)

        tree_catlist_n(node->ncmd.args, sa, " ", depth + 1);

      /* concatenate redirections */
      if(node->ncmd.rdir) {
        tree_catseparator(sa, " ", depth);
        tree_catlist_n(node->ncmd.rdir, sa, " ", depth + 1);
      }

      break;
    }

    /* concatenate pipelines */
    case N_PIPELINE: {
      union node* command;

      for(command = node->npipe.cmds; command; command = command->next) {
        tree_cat(command, sa);

        if(command->next)
          stralloc_cats(sa, " | ");
      }

      break;
    }

    /* assemble an argument and quote it correctly */
    case N_ASSIGN:
    case N_ARG: {
      union node* subarg;
      int prevtable = 0;

      for(subarg = node->narg.list; subarg;) {
        int table = subarg->nargstr.flag & (S_SQUOTED | S_DQUOTED);

        if(table != prevtable && table)
          stralloc_catc(sa, (table == S_DQUOTED ? '"' : '\''));

        tree_cat(subarg, sa);
        prevtable = table;
        subarg = subarg->next;

        table = subarg ? subarg->nargstr.flag & (S_SQUOTED | S_DQUOTED) : 0;

        if(table != prevtable && prevtable)
          stralloc_catc(sa, (prevtable == S_DQUOTED ? '"' : '\''));
      }

      break;
    }

    /* concatenate arguments */
    case N_ARGSTR: {
      size_t i;
      struct nargstr* arg = &node->nargstr;
      int table = arg->flag & S_TABLE;

      for(i = 0; i < arg->len; i++) {
        if(arg->str[i] == '\\' && table == S_SQUOTED) {
          i++;

        } else if(arg->str[i] == '\\' && table == S_DQUOTED && !parse_isdesc(arg->str[i])) {
          continue;
        } else if(!parse_isesc(arg->str[i])) {

          if(table == S_DQUOTED && parse_isdesc(arg->str[i]))
            stralloc_catc(sa, '\\');
        } /*else if(arg->str[i] == '\\') {
          if(++i < arg->len)
            stralloc_catc(sa, arg->str[i]);
          continue;
        }*/

        stralloc_catc(sa, arg->str[i]);
      }

      break;
    }

    /* concatenate variables */
    case N_ARGPARAM: {
      int braces = 0;

      /* if we have a word substitution inside the var we MUST
        put it inside braces */
      if(node->nargparam.word || (node->nargparam.flag & S_STRLEN) ||
         (node->nargparam.flag & S_VAR))
        braces = 1;

      /* use braces if the next char after the variable name
        is a valid name char */
      else if(node->next && node->next->id == N_ARGSTR) {
        stralloc* sa = &node->next->nargstr.stra;

        if((!(node->nargparam.flag & S_SPECIAL) && sa->len && parse_isname(sa->s[0], 0)) ||
           ((node->nargparam.flag & S_SPECIAL) == S_ARG && node->nargparam.numb > 9))
          braces = 1;
      }

      if(!(node->nargparam.flag & S_ARITH) || (node->nargparam.flag & S_SPECIAL) || braces)
        stralloc_catc(sa, '$');

      if(braces)
        stralloc_catc(sa, '{');

      if((node->nargparam.flag & S_STRLEN))
        stralloc_catc(sa, '#');

      /* now print the parameter name and the word expansion */
      if((node->nargparam.flag & S_SPECIAL) == S_ARG)
        stralloc_catulong0(sa, node->nargparam.numb, 0);
      else
        stralloc_cats(sa, node->nargparam.name);

      if(node->nargparam.word || (node->nargparam.flag & S_VAR)) {
        static const char* vsubst_types[] = {
            "-",
            "=",
            "?",
            "+",
            "%",
            "%%",
            "#",
            "##",
        };

#if WITH_PARAM_RANGE
        if((node->nargparam.flag & S_VAR) == S_RANGE) {
          stralloc_catc(sa, ':');

        } else
#endif
        {
          if((node->nargparam.flag & S_NULL))
            stralloc_catc(sa, ':');

          stralloc_cats(sa, vsubst_types[(node->nargparam.flag & S_VAR) >> 8]);
        }

        if(node->nargparam.word)
          tree_cat(node->nargparam.word, sa);
      }

      if(braces)
        stralloc_catc(sa, '}');

      break;
    }

    /* command substitutions */
    case N_ARGCMD: {
      stralloc_cats(sa, (node->nargcmd.flag & S_BQUOTE) ? "`" : "$(");

      if(node->nargcmd.list)
        tree_catlist(node->nargcmd.list, sa, NULL);

      stralloc_catc(sa, (node->nargcmd.flag & S_BQUOTE) ? '`' : ')');

      break;
    }

    /* print if-then-elif-else-fi conditionals */
    case N_IF: {
      union node* n;
    print_if:
      stralloc_cats(sa, "if ");
      tree_catlist(node->nif.test, sa, NULL);

      if(node->nif.cmd0) {
        tree_catseparator(sa, "; then\n", depth + 1);
        tree_catlist_n(node->nif.cmd0, sa, "\n", depth + 1);
      }

      if(node->nif.cmd1) {
        /* handle elif */
        if(node->nif.cmd1->id == N_IF && node->nif.cmd1->next == NULL) {
          tree_catseparator(sa, "\nel", depth);
          node = node->nif.cmd1;
          goto print_if;
        } else if(node->nif.cmd1) {
          tree_catseparator(sa, "\nelse\n  ", depth);
          tree_catlist_n(node->nif.cmd1, sa, "\n", depth + 1);
        }
      }

      tree_catseparator(sa, "\nfi", depth);

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }

      break;
    }

    /* print for-loops */
    case N_FOR: {
      union node* n;
      stralloc_cats(sa, "for ");
      stralloc_cats(sa, node->nfor.varn);

      if(node->nfor.args) {
        stralloc_cats(sa, " in ");
        tree_catlist(node->nfor.args, sa, " ");
      }

      stralloc_cats(sa, "; do ");
      tree_catlist_n(node->nfor.cmds, sa, NULL, depth + 1);

      if(!(sa->len > 1 && sa->s[sa->len - 2] == '&'))
        stralloc_catc(sa, ';');

      if(!(sa->len > 0 && sa->s[sa->len - 1] == ' '))
        stralloc_catc(sa, ' ');

      stralloc_cats(sa, "done");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }

      break;
    }

    /* print while/until-loops */
    case N_WHILE:
    case N_UNTIL: {
      union node* n;
      stralloc_cats(sa, (node->id == N_WHILE ? "while " : "until "));
      tree_catlist(node->nloop.test, sa, NULL);
      tree_catseparator(sa, "; do\n  ", depth);
      tree_catlist_n(node->nloop.cmds, sa, "\n  ", depth + 1);
      tree_catseparator(sa, sep == NULL ? "\n" : sep, depth);
      stralloc_cats(sa, "done");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }

      break;
    }

    case N_CASE: {
      union node* n;
      stralloc_cats(sa, "case ");
      tree_cat(node->ncase.word, sa);
      stralloc_cats(sa, " in");
      tree_catseparator(sa, sep == NULL ? "\n  " : sep, depth);
      tree_catlist_n(node->ncase.list, sa, sep == NULL ? "\n" : sep, depth + 1);
      tree_catseparator(sa, sep == NULL ? "\n" : sep, depth);
      stralloc_cats(sa, "esac");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }
      break;
    }

    case N_CASENODE: {
      tree_catlist(node->ncasenode.pats, sa, "|");
      stralloc_cats(sa, ") ");

      if(node->ncasenode.cmds)
        tree_catlist_n(node->ncasenode.cmds, sa, sep == NULL ? "\n" : sep, depth + 1);

      stralloc_cats(sa, sep == NULL ? " ;;" : ";;");
      break;
    }

    /* print boolean lists */
    case N_NOT: stralloc_cats(sa, "! ");
    case N_AND:
    case N_OR: {
      tree_catlist(node->nandor.left, sa, NULL);

      if(node->id == N_AND || node->id == N_OR) {
        stralloc_cats(sa, node->id == N_AND ? " && " : " || ");
        tree_catlist(node->nandor.right, sa, NULL);
      }

      break;
    }

    case N_LIST: {
      tree_catlist(node->nlist.cmds, sa, node->nlist.bgnd ? " & " : "; ");
      break;
    }

    /* print grouping compounds */
    case N_SUBSHELL: {
      union node* n;
      n = node->ngrp.cmds;

      if(n->id != N_SIMPLECMD || n->next) {
        if(tree_count(n) > 1)
          sep = "\n ";
        else
          sep = 0; //"\n";
      }

      // tree_catseparator(sa, sep == NULL ? " " : sep, depth  - 1);

      if(sa->len && byte_chr(" \t", 2, sa->s[sa->len - 1]) < 2)
        sa->len--;

      stralloc_catc(sa, '(');

      tree_catlist_n(node->ngrp.cmds, sa, sep, depth - 1);
      stralloc_catc(sa, ')');

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }

      break;
    }

    case N_BRACEGROUP: {
      union node* n;
      stralloc_catc(sa, '{');
      int d = depth < 0 ? -depth : depth;

      tree_catseparator(sa, sep == NULL ? "\n  " : sep, d + 1);
      tree_catlist_n(node->ngrp.cmds, sa, sep == NULL ? "; " : sep, d + 1);

      stralloc_cats(sa, ";");

      tree_catseparator(sa, sep == NULL ? "\n  " : sep, d - 1);
      stralloc_catc(sa, '}');

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }

      break;
    }

    /* print redirections operators */
    case N_REDIR: {
      /*      stralloc_catc(sa, ' ');*/

      if(((node->nredir.flag & R_IN) && node->nredir.fdes != 0) ||
         ((node->nredir.flag & R_OUT) && node->nredir.fdes != 1))
        stralloc_catulong0(sa, node->nredir.fdes, 0);

      if(node->nredir.flag & R_IN)
        stralloc_catc(sa, '<');

      if(node->nredir.flag & R_OUT)
        stralloc_catc(sa, '>');

      if(node->nredir.flag & R_APPEND)
        stralloc_catc(sa, '>');

      if(node->nredir.flag & R_DUP)
        stralloc_catc(sa, '&');

      if(node->nredir.flag & R_HERE) {
        stralloc_catc(sa, '<');

        if(node->nredir.flag & R_STRIP)
          stralloc_catc(sa, '-');
      }

      if(node->nredir.flag & R_CLOBBER)
        stralloc_catc(sa, '|');

      if(node->nredir.word)
        tree_catlist(node->nredir.word, sa, "");

      break;
    }

    /* TODO:  IMPLEMENT  !!!*/
    case N_FUNCTION: {
      stralloc_cats(sa, node->nfunc.name);
      stralloc_cats(sa, "() ");

      node = node->nfunc.body;
      sep = "\n";
      // depth++;
      goto again;
      /// tree_cat_n(node->nfunc.body, sa, /*sep == NULL ? "\n  " :sep,*/
      /// depth); break;
    }

    case N_ARGARITH: {
      stralloc_cats(sa, "$((");
      tree_catlist(node->nargarith.tree, sa, NULL);
      stralloc_cats(sa, "))");
      break;
    }

    case A_NUM: {
      char buf[FMT_8LONG];
      size_t n;

      switch(node->narithnum.base) {
        case 8:
          stralloc_catc(sa, '0');
          n = fmt_8longlong(buf, node->narithnum.num);
          break;
        case 16:
          stralloc_cats(sa, "0x");
          n = fmt_xlonglong(buf, node->narithnum.num);
          break;
        case 10:
        default: n = fmt_longlong(buf, node->narithnum.num); break;
      }

      stralloc_catb(sa, buf, n);
      break;
    }

      /*
                case A_VAR: {
                  stralloc_cats(sa, node->narithvar.var);
                  break;
                }*/

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
    case A_EXP: {

      tree_cat(node->narithbinary.left, sa);
      stralloc_catc(sa, ' ');

      switch(node->narithbinary.id) {
        case A_ADD: stralloc_catc(sa, '+'); break;
        case A_SUB: stralloc_catc(sa, '-'); break;
        case A_MUL: stralloc_catc(sa, '*'); break;
        case A_DIV: stralloc_catc(sa, '/'); break;
        case A_OR: stralloc_cats(sa, "||"); break;
        case A_AND: stralloc_cats(sa, "&&"); break;
        case A_BITOR: stralloc_catc(sa, '|'); break;
        case A_BITXOR: stralloc_catc(sa, '^'); break;
        case A_BITAND: stralloc_catc(sa, '&'); break;
        case A_EQ: stralloc_cats(sa, "=="); break;
        case A_NE: stralloc_cats(sa, "!="); break;
        case A_LT: stralloc_cats(sa, "<"); break;
        case A_GT: stralloc_cats(sa, ">"); break;
        case A_GE: stralloc_cats(sa, ">="); break;
        case A_LE: stralloc_cats(sa, "<="); break;
        case A_SHL: stralloc_cats(sa, "<<"); break;
        case A_SHR: stralloc_cats(sa, ">>"); break;
        case A_MOD: stralloc_catc(sa, '%'); break;
        case A_EXP: stralloc_cats(sa, "**"); break;
        default: break;
      }

      stralloc_catc(sa, ' ');
      tree_cat(node->narithbinary.right, sa);
      break;
    }

    case A_PAREN:

      stralloc_catc(sa, '(');
      tree_catlist(node->nargarith.tree, sa, ", ");
      stralloc_catc(sa, ')');
      break;

    case A_NOT:
    case A_BNOT:
    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_PREINCR:
    case A_PREDECR:
    case A_POSTINCR:
    case A_POSTDECR: {

      switch(node->narithunary.id) {
        case A_NOT: stralloc_catc(sa, '!'); break;
        case A_BNOT: stralloc_catc(sa, '~'); break;
        case A_UNARYMINUS: stralloc_catc(sa, '-'); break;
        case A_UNARYPLUS: stralloc_catc(sa, '+'); break;
        case A_PREINCR: stralloc_cats(sa, "++"); break;
        case A_PREDECR: stralloc_cats(sa, "--"); break;
        default: break;
      }

      tree_cat(node->narithunary.node, sa);

      switch(node->narithunary.id) {
        case A_POSTINCR: stralloc_cats(sa, "++"); break;
        case A_POSTDECR: stralloc_cats(sa, "--"); break;
        default: break;
      }

      break;
    }

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
    case A_VBITOR: {

      tree_cat(node->narithbinary.left, sa);
      stralloc_catc(sa, ' ');

      switch(node->narithbinary.id) {
        case A_VASSIGN: stralloc_catc(sa, '='); break;
        case A_VADD: stralloc_cats(sa, "+="); break;
        case A_VSUB: stralloc_cats(sa, "-="); break;
        case A_VMUL: stralloc_cats(sa, "*="); break;
        case A_VDIV: stralloc_cats(sa, "/="); break;
        case A_VMOD: stralloc_cats(sa, "%="); break;
        case A_VSHL: stralloc_cats(sa, "<<="); break;
        case A_VSHR: stralloc_cats(sa, ">>="); break;
        case A_VBITAND: stralloc_cats(sa, "&="); break;
        case A_VBITXOR: stralloc_cats(sa, "^="); break;
        case A_VBITOR: stralloc_cats(sa, "|="); break;
        default: break;
      }

      stralloc_catc(sa, ' ');
      tree_cat(node->narithbinary.right, sa);
      break;
    }
  }
}
