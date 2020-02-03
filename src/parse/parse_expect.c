#include "debug.h"
#include "parse.h"
#include "tree.h"

/* expect a token, print error msg and return 0 if it wasn't that token
 * ----------------------------------------------------------------------- */
int
parse_expect(struct parser* p, int tempflags, enum tok_flag toks, union node* nfree) {
  if(!(parse_gettok(p, tempflags) & toks)) {
    parse_error(p, toks);

    if(nfree) {
#ifdef DEBUG_OUTPUT
      debug_list(nfree, 0);
#endif /* DEBUG */
      tree_free(nfree);
    }

    return 0;
  }

  return p->tok;
}
