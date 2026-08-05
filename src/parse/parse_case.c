#include "../parse.h"
#include "../tree.h"

/* 3.9.4.3 - parse case statement
 *
 *  The format for the case construct is as follows.
 *
 *        case word in
 *             [(]pattern1)          compound-list;;
 *             [(]pattern2|pattern3) compound-list;;
 *             ...
 *        esac
 *
 *  The ;; is optional for the last compound-list.
 *
 * ----------------------------------------------------------------------- */
union node*
parse_case(struct parser* p) {
  union node *node, **cptr, **pptr, *word;

  /* next tok must be a word */
  if(!parse_expect(p, P_DEFAULT, T_WORD | T_NAME | T_ASSIGN, NULL))
    return NULL;

  /* create new node and move the word to it */
  node = tree_newnode(N_CASE);
  node->ncase.word = word = parse_getarg(p);

  /* then the keyword 'in' must follow */

  if(!parse_expect(p, P_DEFAULT, T_IN, node))
    return NULL;

  /* initialize tree for the cases */
  tree_init(node->ncase.list, cptr);

  /* parse the cases */
  while(!(parse_gettok(p, P_SKIPNL) & T_ESAC)) {
    /* patterns may be introduced with '(' */
    if(!(p->tok & T_LP))
      p->pushback++;

    *cptr = tree_newnode(N_CASENODE);
    tree_init((*cptr)->ncasenode.pats, pptr);

    /* parse the pattern list -- P_NOKEYWD: a case pattern is not a
       keyword-recognition position (POSIX 2.10.2's reserved-word
       rule only applies where a keyword is grammatically expected),
       so a pattern alternative that happens to spell a reserved word
       verbatim (e.g. libtool's "finish|finis|fini|fin|fi|f)") must
       still parse as a plain word, not the token that ends an
       enclosing "if" (case-pattern-word-treated-as-keyword). Same
       fix already applied to a "for ... in word-list" list's own
       words, see parse_for.c. */
    while(parse_gettok(p, P_SKIPNL | P_NOKEYWD) & (T_WORD | T_NAME | T_ASSIGN | T_LP)) {
      if(p->tok == T_LP)
        continue;

      *pptr = parse_getarg(p);
      pptr = tree_next(pptr);

      if(!(parse_gettok(p, P_DEFAULT) & T_PIPE))
        break;
    }

    p->pushback++;

    if(!parse_expect(p, P_DEFAULT, T_RP | T_PIPE, node))
      return NULL;

    /* parse the compound list */
    (*cptr)->ncasenode.cmds = parse_compound_list(p, T_ESAC | T_ECASE);

    /* expect esac or ;; */
    if(!parse_expect(p, P_DEFAULT, T_ESAC | T_ECASE, node))
      return NULL;

    if(p->tok & T_ESAC)
      break;

    tree_skip(cptr);
  }

  return node;
}
