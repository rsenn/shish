#include "../parse.h"
#include "../../lib/scan.h"
#include "../source.h"
#include "../tree.h"

/* parse a word token
 * ----------------------------------------------------------------------- */
int
parse_word(struct parser* p) {
  char c;

  /* if there is still a tree from the last call then remove it */
  if(p->tree)
    tree_free(p->tree);

  p->tree = NULL;
  p->node = NULL;
  p->quot = Q_UNQUOTED;
  p->tokstart = source->position;

  /* initialize string data */
  stralloc_zero(&p->sa);

  p->tok = T_NAME;

  for(;;) {
    if(source_peek(&c) <= 0)
      break;

    switch(p->quot) {
      case Q_DQUOTED:
        if(!parse_dquoted(p))
          continue;
        break;
      case Q_SQUOTED:
        if(!parse_squoted(p))
          continue;
        break;
      default:
        if(!parse_unquoted(p))
          continue;
        break;
    }

    break;
  }
  /*
    if(p->sa.len == 0) {
      len = source->b->p - p->tokstart;

if(len > 0)
        stralloc_catb(&p->sa, &source->b->x[p->tokstart], len);
    }
   */

  /* mirror parse_unquoted()'s delimiter-branch keyword check: normally
     a keyword is recognized right when a following delimiter char
     gets peeked (see parse_unquoted.c), *before* parse_string() below
     clears p->sa into the finished node. At true end-of-input there's
     no delimiter char left to peek -- the loop above just breaks --
     so without this, a word ending exactly at EOF (no trailing
     whitespace/newline, e.g. "done"/"fi" as the very last bytes of a
     "-c" argument, which unlike a script file has no trailing
     newline) never gets a chance to be recognized as a keyword before
     parse_string() wipes p->sa clean. */
  if(p->quot == Q_UNQUOTED && !(p->flags & P_NOKEYWD) && !p->tree && p->sa.s)
    parse_keyword(p);

  parse_string(p, 0);
  return p->tok;
}
