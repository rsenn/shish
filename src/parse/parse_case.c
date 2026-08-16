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

  /* next tok must be a word -- P_NOKEYWD: "case if in ...' is valid
     (confirmed against bash/dash), since this word position isn't one
     of POSIX 2.10.2's reserved-word-recognition positions (it doesn't
     start a command); same reasoning as parse_for.c's own list word */
  if(!parse_expect(p, P_NOKEYWD, T_WORD | T_NAME | T_ASSIGN, NULL))
    return NULL;

  /* create new node and move the word to it */
  node = tree_newnode(N_CASE);
  node->ncase.word = word = parse_getarg(p);

  /* then the keyword 'in' must follow -- P_SKIPNL: a linebreak
     between the case word and 'in' is allowed (3.9.4.3's grammar has
     a <linebreak> right there), same as parse_for.c's own linebreak
     before 'do' */
  if(!parse_expect(p, P_SKIPNL, T_IN, node))
    return NULL;

  /* initialize tree for the cases */
  tree_init(node->ncase.list, cptr);

  /* parse the cases */
  while(!(parse_gettok(p, P_SKIPNL) & T_ESAC)) {
    /* patterns may be introduced with '(' */
    if(!(p->tok & T_LP)) {
      /* the token just fetched recognizes keywords (needed for
         "esac"), so a pattern spelling some *other* reserved word
         verbatim (e.g. "if)") is already resolved to that keyword.
         Downgrade it back to T_NAME -- parse_keyword() never touches
         p->sa, only p->tok, so the raw text is still intact. */
      if(p->tok != T_WORD && p->tok != T_NAME && p->tok != T_ASSIGN)
        p->tok = T_NAME;

      p->pushback++;
    }

    *cptr = tree_newnode(N_CASENODE);
    tree_init((*cptr)->ncasenode.pats, pptr);

    /* parse the pattern list -- P_NOKEYWD: a case pattern is not a
       keyword-recognition position, so an alternative that happens to
       spell a reserved word verbatim (e.g. "finish|finis|fin|fi|f)")
       still parses as a plain word, same as parse_for.c's list. */
    while(parse_gettok(p, P_SKIPNL | P_NOKEYWD) & (T_WORD | T_NAME | T_ASSIGN | T_LP)) {
      if(p->tok == T_LP)
        continue;

      *pptr = parse_getarg(p);
      pptr = tree_next(pptr);

      if(!(parse_gettok(p, P_NOKEYWD) & T_PIPE))
        break;
    }

    p->pushback++;

    if(!parse_expect(p, P_NOKEYWD, T_RP | T_PIPE, node))
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
