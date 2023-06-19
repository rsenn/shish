#include "../parse.h"
#include "../tree.h"

/* parse a compound list
 *
 * The term compound-list is derived from the grammar in 3.10; it is
 * equivalent to a sequence of lists, separated by <newline>s, that
 * can be preceded or followed by an arbitrary number of <newline>s.
 * ----------------------------------------------------------------------- */
union node*
parse_compound_list(struct parser* p, enum tok_flag end_tok) {
  union node* list;
  union node** nptr;

  tree_init(list, nptr);

  for(;;) {
    if(end_tok == T_END)
      p->flags &= ~(P_NOKEYWD | P_SKIPNL);

    /* skip arbitrary newlines */
    while(parse_gettok(p, P_DEFAULT) & T_NL)
      ;
    p->pushback++;

    if(parse_gettok(p, P_DEFAULT) == end_tok) {
      p->pushback++;
      break;
    }

    p->pushback++;

    /* try to parse a list */
    *nptr = parse_list(p);

    /* skip arbitrary newlines */
    while(p->tok & T_NL)
      parse_gettok(p, P_DEFAULT);
    p->pushback++;
    /*
        if(end_tok != -1 && (p->tok & end_tok))
          break; */

    /* no more lists */
    if(*nptr == NULL)
      break;

    /* parse_list already returns a list, so we
       must skip over it to get &lastnode->next */
    while(*nptr)
      tree_skip(nptr);
  }

  return list;
}
