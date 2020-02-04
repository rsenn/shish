#include "../parse.h"
#include "../tree.h"

/* parse a compound list
 *
 * The term compound-list is derived from the grammar in 3.10; it is
 * equivalent to a sequence of lists, separated by <newline>s, that
 * can be preceded or followed by an arbitrary number of <newline>s.
 * ----------------------------------------------------------------------- */
union node*
parse_compound_list(struct parser* p) {
  union node* list;
  union node** nptr;

  tree_init(list, nptr);

  /* skip arbitrary newlines */
  while(parse_gettok(p, P_DEFAULT) & T_NL)
    ;
  p->pushback++;

  for(;;) {
    /* try to parse a list */
    *nptr = parse_list(p);

    /* skip arbitrary newlines */
    while(p->tok & T_NL) parse_gettok(p, P_DEFAULT);
    p->pushback++;

    /* no more lists */
    if(*nptr == NULL)
      break;

    /* parse_list already returns a list, so we
       must skip over it to get &lastnode->next */
    while(*nptr) tree_next(nptr);
  }

  return list;
}
