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

    assert(m->id == N_ARGSTR);
    assert(m->nargstr.stra.s);

    wordlist_cat(&wl, m->nargstr.stra.s, m->nargstr.stra.len, lflags | X_NOSPLIT);
  }

  wordlist_close(&wl);
}
