#include "../debug.h"
#include "../parse.h"
#include "../tree.h"

/* expect a token, print error msg and return 0 if it wasn't that token
 * ----------------------------------------------------------------------- */
int
parse_expect(struct parser* p, int tempflags, enum tok_flag toks, union node* nfree) {
  if(!(parse_gettok(p, tempflags) & toks)) {
    parse_error(p, toks);
#ifdef DEBUG_OUTPUT
    if(p->tree)
      debug_list(p->tree, 0);
    if(p->node)
      debug_node(p->node, 0);
#endif /* DEBUG_OUTPUT */
    if(nfree) {
#ifdef DEBUG_OUTPUT
      debug_list(nfree, 0);
#endif /* DEBUG_OUTPUT */
      tree_free(nfree);
    }

    return 0;
  }

  return p->tok;
}
