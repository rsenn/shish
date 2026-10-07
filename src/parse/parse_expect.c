#include "../debug.h"
#include "../trace.h"
#include "../parse.h"
#include "../tree.h"

/* a closing keyword (fi, done, esac, }) was just lexed as a word: its node is of no further use,
 * and nothing frees it if the input ends here (the next word would)
 * ----------------------------------------------------------------------- */
void
parse_dropkeyword(struct parser* p) {
  if(p->tree)
    tree_free(p->tree);

  p->tree = p->node = NULL;
}

/* expect a token, print error msg and return 0 if it wasn't that token
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_expect(struct parser* p, int tempflags, enum tok_flag toks, union node* nfree) {
  if(!(parse_gettok(p, tempflags) & toks)) {
    parse_error(p, toks);

    if(nfree) {
      TRACE(TRACE_PARSE, "expect_failed", trace_nodes("discarded", nfree));
      tree_free(nfree);
    }

    return 0;
  }

  p->pushback = 0;

  if(p->tok & (T_FI | T_DONE | T_ESAC | T_END))
    parse_dropkeyword(p);

  return p->tok;
}
