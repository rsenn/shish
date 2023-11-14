#include "../expand.h"
#include "../tree.h"
#include <assert.h>

/* expand all parts of an N_ARG node
 * ----------------------------------------------------------------------- */
void
expand_str(union node* node, stralloc* sa, int flags) {
  union node *n = 0, *subarg;

  /* loop through all parts of the word */
  for(subarg = (node && node->id == N_ARG) ? node->narg.list : node; subarg;
      subarg = subarg->next) {
    int lflags = flags; /* local flags */

    if(subarg->nargstr.flag & S_TABLE)
      lflags |= X_QUOTED;

    if(subarg->nargstr.flag & S_GLOB)
      lflags |= X_GLOB;

    assert(subarg->id == N_ARGSTR);
    assert(subarg->nargstr.stra.s);

    expand_cat(subarg->nargstr.stra.s,
               subarg->nargstr.stra.len,
               &n,
               lflags | X_NOSPLIT);
  }

  stralloc_move(sa, &n->narg.stra);
  tree_free(n);
}
