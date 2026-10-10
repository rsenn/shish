#include "../expand.h"
#include "../tree.h"
#include <assert.h>

/* expand all parts of an N_ARG node of plain text into sa (overwritten)
 * ----------------------------------------------------------------------- */
void
expand_str(union node* node, stralloc* sa, int flags) {
  union node* m;
  wordlist wl;

  stralloc_zero(sa);
  wordlist_init_str(&wl, sa);

  /* loop through all parts of the word */
  for(m = (node && node->id == N_ARG) ? node->narg.list : node; m; m = m->next) {
    int lflags = flags; /* local flags */

    if(m->nargstr.flag & S_TABLE)
      lflags |= X_QUOTED;

    if(m->nargstr.flag & S_GLOB)
      lflags |= X_GLOB;

    /* the parser doubles backslashes in source text: "X Y" is stored as X\ Y */
    if(m->nargstr.len && !(m->nargstr.flag & S_HEREDOC))
      lflags |= X_LITERAL;

    assert(m->id == N_ARGSTR);
    assert(m->nargstr.s);

    wordlist_cat(&wl, m->nargstr.s, m->nargstr.len, lflags | X_NOSPLIT);
  }

  wordlist_close(&wl);
}
