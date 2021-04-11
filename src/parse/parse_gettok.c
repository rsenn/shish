#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../parse.h"
#include "../fd.h"
#include "../tree.h"
#include "../source.h"
#include "../sh.h"
#include "../debug.h"

/* get a token, the argument indicates whether to search for keywords or not
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_gettok(struct parser* p, int tempflags) {
  int oldflags = p->flags;
  p->flags |= tempflags;

  if(!p->pushback || ((p->flags & P_SKIPNL) && p->tok == T_NL)) {
    p->tok = -1;
    /* skip whitespace */
    // p->tok = parse_skipspace(p);
    p->tokstart = source->position;

    if(p->tree && p->tree->id == N_ARGSTR)
      stralloc_zero(&p->tree->nargstr.stra);

    stralloc_zero(&p->sa);
    /* check for simple tokens first */
    if(p->tok == -1)
      p->tok = parse_simpletok(p);
    /* and then for words */
    if(p->tok == -1)
      p->tok = parse_word(p);
    /* if the token is a valid name then it could be a keyword */
    if(p->tok & T_NAME && p->node && p->node->id == N_ARGSTR && !(p->flags & P_NOKEYWD))
      parse_keyword(p);

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE)
    if(sh->opts.debug) {
      if(p->tok != -1)
        parse_dump(p, &debug_buffer);
    }
#endif
  }

  p->flags = oldflags;
  p->pushback = 0;
  return p->tok;
}
