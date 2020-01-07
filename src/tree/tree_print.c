#include "expand.h"
#include "fmt.h"
#include "parse.h"
#include "redir.h"
#include "tree.h"
#include <stdlib.h>

/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_print(union node* node, stralloc* sa) {
  switch(node->id) {
    case N_SIMPLECMD: {
      union node* n;

      /* concatenate vars */
      for(n = node->ncmd.vars; n; n = n->list.next) {
        tree_print(n, sa);
        if(n->list.next || node->ncmd.args)
          stralloc_catc(sa, ' ');
      }

      /* concatenate arguments */
      for(n = node->ncmd.args; n; n = n->list.next) {
        tree_print(n, sa);
        if(n->list.next)
          stralloc_catc(sa, ' ');
      }

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
      }
      break;
    }

    /* concatenate pipelines */
    case N_PIPELINE: {
      union node* command;

      for(command = node->npipe.cmds; command; command = command->list.next) {
        tree_print(command, sa);

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

        tree_print(subarg, sa);
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

      stralloc_catc(sa, '$');

      /* if we have a word substitution inside the var we MUST
        put it inside braces */
      if(node->nargparam.word || (node->nargparam.flag & S_STRLEN)) {
        braces = 1;
      }
      /* use braces if the next char after the variable name
        is a valid name char */
      else if(node->list.next && node->list.next->id == N_ARGSTR) {
        stralloc* sa = &node->list.next->nargstr.stra;

        if((!(node->nargparam.flag & S_SPECIAL) && sa->len && parse_isname(sa->s[0])) || node->nargparam.numb > 9)
          braces = 1;
      }

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

        tree_print(node->nargparam.word, sa);
      }

      if(braces)
        stralloc_catc(sa, '}');

      break;
    }

    /* command substitutions */
    case N_ARGCMD: {
      stralloc_cats(sa, (node->nargcmd.flag & S_BQUOTE) ? "`" : "$(");

      if(node->nargcmd.list)
        tree_printlist(node->nargcmd.list, sa, NULL);

      stralloc_catc(sa, (node->nargcmd.flag & S_BQUOTE) ? '`' : ')');

      break;
    }

    /* print if-then-elif-else-fi conditionals */
    case N_IF: {
      union node* n;
    print_if:
      stralloc_cats(sa, "if ");
      tree_printlist(node->nif.test, sa, NULL);

      if(node->nif.cmd0) {
        stralloc_cats(sa, "; then ");
        tree_printlist(node->nif.cmd0, sa, NULL);
      }
      if(node->nif.cmd1) {
        /* handle elif */
        if(node->nif.cmd1->id == N_IF && node->nif.cmd1->list.next == NULL) {
          stralloc_cats(sa, "; el");
          node = node->nif.cmd1;
          goto print_if;
        } else if(node->nif.cmd1) {
          stralloc_cats(sa, "; else ");
          tree_printlist(node->nif.cmd1, sa, NULL);
        }
      }

      stralloc_cats(sa, "; fi");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
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
        tree_printlist(node->nfor.args, sa, " ");
      }
      stralloc_cats(sa, "; do ");
      tree_printlist(node->nfor.cmds, sa, NULL);
      stralloc_cats(sa, "; done");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
      }
      break;
    }

    /* print while/until-loops */
    case N_WHILE:
    case N_UNTIL: {
      union node* n;
      stralloc_cats(sa, (node->id == N_WHILE ? "while " : "until "));
      tree_printlist(node->nloop.test, sa, NULL);
      stralloc_cats(sa, "; do ");
      tree_printlist(node->nloop.cmds, sa, NULL);
      stralloc_cats(sa, "; done");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
      }
      break;
    }

    case N_CASE: {
      union node* n;
      stralloc_cats(sa, "case ");
      tree_print(node->ncase.word, sa);
      stralloc_cats(sa, " in; ");
      tree_printlist(node->ncase.list, sa, " ;; ");
      stralloc_cats(sa, ";; esac");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
      }
      break;
    }

    case N_CASENODE: {
      tree_printlist(node->ncasenode.pats, sa, "|");
      stralloc_cats(sa, ") ");
      if(node->ncasenode.cmds)
        tree_printlist(node->ncasenode.cmds, sa, NULL);
      break;
    }

    /* print boolean lists */
    case N_NOT: stralloc_cats(sa, "! ");
    case N_AND:
    case N_OR: {
      tree_printlist(node->nandor.cmd0, sa, NULL);

      if(node->id == N_AND || node->id == N_OR) {
        stralloc_cats(sa, node->id == N_AND ? " && " : " || ");
        tree_printlist(node->nandor.cmd1, sa, NULL);
      }

      break;
    }

    /* print grouping compounds */
    case N_SUBSHELL: {
      union node* n;
      stralloc_catc(sa, '(');
      tree_printlist(node->ngrp.cmds, sa, NULL);
      stralloc_catc(sa, ')');

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
      }
      break;
    }
    case N_CMDLIST: {
      union node* n;
      stralloc_cats(sa, "{ ");
      tree_printlist(node->ngrp.cmds, sa, NULL);
      stralloc_cats(sa, "; }");

      /* concatenate redirections */
      for(n = node->ncmd.rdir; n; n = n->list.next) {
        stralloc_catc(sa, ' ');
        tree_print(n, sa);
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
        tree_printlist(node->nredir.list, sa, "");

      break;
    }

    /* TODO:  IMPLEMENT  !!!*/
    case N_FUNCTION: break;

    case N_ARGARITH: {
      stralloc_cats(sa, "$((");
      tree_printlist(node->nargarith.tree, sa, NULL);
      stralloc_cats(sa, "))");
      break;
    }

    case N_ARITH_NUM: {
      stralloc_catlong(sa, node->narithnum.num);
      break;
    }
      /*
          case N_ARITH_VAR: {
            stralloc_cats(sa, node->narithvar.var);
            break;
          }*/

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
    case N_ARITH_REM:
    case N_ARITH_EXP: {

      tree_print(node->narithbinary.left, sa);

      switch(node->narithbinary.id) {
        case N_ARITH_ADD: stralloc_catc(sa, '+'); break;
        case N_ARITH_SUB: stralloc_catc(sa, '-'); break;
        case N_ARITH_MUL: stralloc_catc(sa, '*'); break;
        case N_ARITH_DIV: stralloc_catc(sa, '/'); break;
        case N_ARITH_OR: stralloc_cats(sa, "||"); break;
        case N_ARITH_AND: stralloc_cats(sa, "&&"); break;
        case N_ARITH_BOR: stralloc_catc(sa, '|'); break;
        case N_ARITH_BXOR: stralloc_catc(sa, '^'); break;
        case N_ARITH_BAND: stralloc_catc(sa, '&'); break;
        case N_ARITH_EQ: stralloc_cats(sa, "=="); break;
        case N_ARITH_NE: stralloc_cats(sa, "!="); break;
        case N_ARITH_LT: stralloc_cats(sa, "<"); break;
        case N_ARITH_GT: stralloc_cats(sa, ">"); break;
        case N_ARITH_GE: stralloc_cats(sa, ">="); break;
        case N_ARITH_LE: stralloc_cats(sa, "<="); break;
        case N_ARITH_LSHIFT: stralloc_cats(sa, "<<"); break;
        case N_ARITH_RSHIFT: stralloc_cats(sa, ">>"); break;
        case N_ARITH_REM: stralloc_catc(sa, '%'); break;
        case N_ARITH_EXP: stralloc_cats(sa, "**"); break;
      }

      tree_print(node->narithbinary.right, sa);
      break;
    }

    case N_ARITH_PAREN:

      stralloc_catc(sa, '(');
      tree_printlist(node->nargarith.tree, sa, ", ");
      stralloc_catc(sa, ')');
      break;

    case N_ARITH_NOT:
    case N_ARITH_BNOT:
    case N_ARITH_UNARYMINUS:
    case N_ARITH_UNARYPLUS: {

      switch(node->narithunary.id) {
        case N_ARITH_NOT: stralloc_catc(sa, '!'); break;
        case N_ARITH_BNOT: stralloc_catc(sa, '~'); break;
        case N_ARITH_UNARYMINUS: stralloc_catc(sa, '-'); break;
        case N_ARITH_UNARYPLUS: stralloc_catc(sa, '+'); break;
      }
      tree_print(node->narithunary.node, sa);
      break;
    }
  }
}
