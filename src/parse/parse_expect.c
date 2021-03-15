#include "../debug.h"
#include "../parse.h"
#include "../tree.h"

/* expect a token, print error msg and return 0 if it wasn't that token
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_expect(struct parser* p,
             int tempflags,
             enum tok_flag toks,
             union node* nfree) {
  if(!(parse_gettok(p, tempflags) & toks)) {
    parse_error(p, toks);
    if(nfree) {
#ifdef DEBUG_PARSE
      debug_list(nfree, 0);
#endif /* DEBUG_PARSE */
      tree_free(nfree);
    }

    return 0;
  }
  p->pushback = 0;
  return p->tok;
}
