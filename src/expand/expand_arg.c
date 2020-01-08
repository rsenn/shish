#include "expand.h"
#include "tree.h"
#include <stdlib.h>

/* expand all parts of an N_ARG node
 * ----------------------------------------------------------------------- */
union node*
expand_arg(struct expand* ex, struct narg* narg) {
  union node* n = ex->ptr;
  union node* subarg;

  /* loop through all parts of the word */
  for(subarg = narg ? narg->list : NULL; subarg; subarg = subarg->list.next) {
    int lflags = ex->flags; /* local ex->flags */

    if(subarg->nargstr.flag & S_NOSPLIT)
      lflags |= X_NOSPLIT;
    if(subarg->nargstr.flag & S_TABLE)
      lflags |= X_QUOTED;
    if(subarg->nargstr.flag & S_GLOB)
      lflags |= X_GLOB;

    /* expand argument parts */
    switch(subarg->id) {
      /* exprmetic substitution */
      case N_ARGARITH: n = expand_arith(ex, &subarg->nargarith); break;

      /* parameter substitution */
      case N_ARGPARAM: n = expand_param(ex, &subarg->nargparam); break;

      /* command substitution */
      case N_ARGCMD: n = expand_command(ex, &subarg->nargcmd); break;

      /* constant string */
      default: n = expand_cat(ex, subarg->nargstr.stra.s, subarg->nargstr.stra.len); break;
    }

    if(n)
      ex->ptr = &n;
  }

  return n;
}
