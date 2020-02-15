#include "../../lib/buffer.h"
#include "../parse.h"
#include "../tree.h"

/* get a token, the argument indicates whether to search for keywords or not
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_gettok(struct parser* p, int tempflags) {
  int oldflags = p->flags;
  p->flags |= tempflags;

  if(!p->pushback || ((p->flags & P_SKIPNL) && p->tok == T_NL)) {
    p->tok = -1;

    /* skip whitespace */
    p->tok = parse_skipspace(p);

    /*  if(!parse_keyword(p))
        p->tok = -1;
  */
    /* check for simple tokens first */
    if(p->tok == -1)
      p->tok = parse_simpletok(p);

    /* and finally for words */
    if(p->tok == -1)
      p->tok = parse_word(p);

    /* if the token is a valid name then it could be a keyword */
    if(p->tok & (T_WORD|T_NAME) && p->node && !(p->flags & P_NOKEYWD))
      parse_keyword(p);
  }
  p->flags = oldflags;
  p->pushback = 0;
  return p->tok;
}
