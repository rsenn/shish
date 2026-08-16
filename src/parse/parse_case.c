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
      /* the token just fetched above has to be tokenized without
         P_NOKEYWD (it needs to recognize "esac" to know when to stop
         the loop), so if it spells some *other* reserved word
         verbatim -- e.g. a pattern like "if)" -- it's already been
         resolved to that keyword's own token bit by the time we get
         here, before the pattern-word loop below (which does request
         P_NOKEYWD) ever gets a chance to ask for it as a plain word
         instead: parse_gettok() only re-tokenizes when !p->pushback,
         so pushing back an already-keyword-resolved token doesn't
         retroactively un-resolve it. parse_keyword() never touches
         p->sa (the raw text backing this token), only p->tok, so
         downgrading it back to T_NAME here -- now that we know for
         certain this position wants a word, not a keyword -- recovers
         a plain word from that same text instead of leaving a stale
         keyword token for the loop below to choke on
         (reserved-words-as-case-patterns). */
      if(p->tok != T_WORD && p->tok != T_NAME && p->tok != T_ASSIGN)
        p->tok = T_NAME;

      p->pushback++;
    }

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
