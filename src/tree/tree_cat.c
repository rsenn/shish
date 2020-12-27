#include "../expand.h"
#include "../../lib/fmt.h"
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
      union node* n;

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

      for(command = node->npipe.cmds; command; command = command->list.next) {
        tree_cat(command, sa);

        if(command->list.next)
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
        int table = subarg->nargstr.flag & S_TABLE;

        if(table != prevtable && table)
          stralloc_catc(sa, (table == S_DQUOTED ? '"' : '\''));

        tree_cat(subarg, sa);
        prevtable = table;
        subarg = subarg->list.next;

        table = subarg ? subarg->nargstr.flag & S_TABLE : 0;

        if(table != prevtable && prevtable)
          stralloc_catc(sa, (prevtable == S_DQUOTED ? '"' : '\''));
      }

      break;
    }

    /* concatenate arguments */
    case N_ARGSTR: {
      int i;
      stralloc* arg = &node->nargstr.stra;

      for(i = 0; i < arg->len; i++) {
        if(!parse_isesc(arg->s[i])) {
          if(arg->s[i] == '\\' && !parse_isdesc(arg->s[i]))
            continue;
          if((node->nargstr.flag & S_TABLE) == S_DQUOTED && parse_isdesc(arg->s[i]))
            stralloc_catc(sa, '\\');
        } else if(arg->s[i] == '\\') {
          if(++i < arg->len)
            stralloc_catc(sa, arg->s[i]);
          continue;
        }

        stralloc_catc(sa, arg->s[i]);
      }
      break;
    }

    /* concatenate variables */
    case N_ARGPARAM: {
      int braces = 0;

      /* if we have a word substitution inside the var we MUST
        put it inside braces */
      if(node->nargparam.word || (node->nargparam.flag & S_STRLEN)) {
        braces = 1;
      }
      /* use braces if the next char after the variable name
        is a valid name char */
      else if(node->list.next && node->list.next->id == N_ARGSTR) {
        stralloc* sa = &node->list.next->nargstr.stra;

        if((!(node->nargparam.flag & S_SPECIAL) && sa->len && parse_isname(sa->s[0], 0)) || node->nargparam.numb > 9)
          braces = 1;
      }

      if(!(node->nargparam.flag & S_ARITH) || braces)
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

      if(node->nargparam.word) {
        static const char* vsubst_types[] = {"-", "=", "?", "+", "%", "%%", "#", "##"};

        if(node->nargparam.flag & S_NULL)
          stralloc_catc(sa, ':');

        stralloc_cats(sa, vsubst_types[(node->nargparam.flag & S_VAR) >> 8]);

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
        if(node->nif.cmd1->id == N_IF && node->nif.cmd1->list.next == NULL) {
          tree_catseparator(sa, "\nel", depth);
          node = node->nif.cmd1;
          goto print_if;
        } else if(node->nif.cmd1) {
          tree_catseparator(sa, "\nelse\n  ", depth);
          tree_catlist_n(node->nif.cmd1, sa, "\n", depth + 1);
        }
      }
      tree_catseparator(sa, "\nfi\n", depth);

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
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
      stralloc_cats(sa, "; done");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
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
      stralloc_cats(sa, "; do ");
      tree_catlist_n(node->nloop.cmds, sa, NULL, depth + 1);
      stralloc_cats(sa, "; done");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
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
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }
      break;
    }

    case N_CASENODE: {
      tree_catlist(node->ncasenode.pats, sa, "|");
      stralloc_cats(sa, ") ");
      if(node->ncasenode.cmds)
        tree_catlist_n(node->ncasenode.cmds, sa, "\n", depth + 1);

      stralloc_cats(sa, sep == NULL ? " ;;" : ";;");
      break;
    }

    /* print boolean lists */
    case N_NOT: stralloc_cats(sa, "! ");
    case N_AND:
    case N_OR: {
      tree_catlist(node->nandor.cmd0, sa, NULL);

      if(node->id == N_AND || node->id == N_OR) {
        stralloc_cats(sa, node->id == N_AND ? " && " : " || ");
        tree_catlist(node->nandor.cmd1, sa, NULL);
      }

      break;
    }

    /* print grouping compounds */
    case N_SUBSHELL: {
      union node* n;
      n = node->ngrp.cmds;

      if(n->id != N_SIMPLECMD || n->list.next)
        sep = "\n";

      stralloc_catc(sa, '(');

      tree_catlist_n(node->ngrp.cmds, sa, sep, depth - 1);
      stralloc_catc(sa, ')');

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_cat(n, sa);
      }
      break;
    }
    case N_CMDLIST: {
      union node* n;
      // stralloc_catc(sa, '{');
      int d = depth < 0 ? -depth : depth;

      tree_catseparator(sa, sep == NULL ? "{\n" : sep, d);
      tree_catlist_n(node->ngrp.cmds, sa, sep == NULL ? "; " : sep, d + 1);
      if(depth < 0)
        stralloc_cats(sa, "\n}");
      else
        tree_catseparator(sa, sep == NULL ? "; }" : sep, d);

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
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

      if(node->nredir.list)
        tree_catlist(node->nredir.list, sa, "");

      break;
    }

    /* TODO:  IMPLEMENT  !!!*/
    case N_FUNCTION: {
      stralloc_cats(sa, node->nfunc.name);
      stralloc_cats(sa, "() ");

      node = node->nfunc.body;
      sep = "\n";
      depth++;
      goto again; /*
 tree_cat_n(node->nfunc.body, sa, -(depth + 1));
 break;*/
    }

    case N_ARGARITH: {
      stralloc_cats(sa, "$((");
      tree_catlist(node->nargarith.tree, sa, NULL);
      stralloc_cats(sa, "))");
      break;
    }

    case A_NUM: {
      stralloc_catlong(sa, node->narithnum.num);
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
    case A_EXP: {

      tree_cat(node->narithbinary.left, sa);

      switch(node->narithbinary.id) {
        case A_ADD: stralloc_catc(sa, '+'); break;
        case A_SUB: stralloc_catc(sa, '-'); break;
        case A_MUL: stralloc_catc(sa, '*'); break;
        case A_DIV: stralloc_catc(sa, '/'); break;
        case A_OR: stralloc_cats(sa, "||"); break;
        case A_AND: stralloc_cats(sa, "&&"); break;
        case A_BOR: stralloc_catc(sa, '|'); break;
        case A_BXOR: stralloc_catc(sa, '^'); break;
        case A_BAND: stralloc_catc(sa, '&'); break;
        case A_EQ: stralloc_cats(sa, "=="); break;
        case A_NE: stralloc_cats(sa, "!="); break;
        case A_LT: stralloc_cats(sa, "<"); break;
        case A_GT: stralloc_cats(sa, ">"); break;
        case A_GE: stralloc_cats(sa, ">="); break;
        case A_LE: stralloc_cats(sa, "<="); break;
        case A_LSHIFT: stralloc_cats(sa, "<<"); break;
        case A_RSHIFT: stralloc_cats(sa, ">>"); break;
        case A_MOD: stralloc_catc(sa, '%'); break;
        case A_EXP: stralloc_cats(sa, "**"); break;
        default: break;
      }

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
    case A_PREINCREMENT:
    case A_PREDECREMENT:
    case A_POSTINCREMENT:
    case A_POSTDECREMENT: {

      switch(node->narithunary.id) {
        case A_NOT: stralloc_catc(sa, '!'); break;
        case A_BNOT: stralloc_catc(sa, '~'); break;
        case A_UNARYMINUS: stralloc_catc(sa, '-'); break;
        case A_UNARYPLUS: stralloc_catc(sa, '+'); break;
        case A_PREINCREMENT: stralloc_cats(sa, "++"); break;
        case A_PREDECREMENT: stralloc_cats(sa, "--"); break;
        default: break;
      }
      tree_cat(node->narithunary.node, sa);
      switch(node->narithunary.id) {
        case A_POSTINCREMENT: stralloc_cats(sa, "++"); break;
        case A_POSTDECREMENT: stralloc_cats(sa, "--"); break;
        default: break;
      }
      break;
    }
  }
}
