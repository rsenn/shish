#include <stdlib.h>
#include "tree.h"
#include "expand.h"

/* expand all parts of an N_ARG node
 * ----------------------------------------------------------------------- */
union node *expand_arg(struct narg *narg, union node **nptr, int flags) {
  union node *n = *nptr;
  union node *subarg;

  /* loop through all parts of the word */
  for(subarg = narg ? narg->list : NULL; subarg; subarg = subarg->list.next) {
    int lflags = flags; /* local flags */
  
    if(subarg->nargstr.flag & S_NOSPLIT)
      lflags |= X_NOSPLIT;
    if(subarg->nargstr.flag & S_TABLE)
      lflags |= X_QUOTED;
    if(subarg->nargstr.flag & S_GLOB)
      lflags |= X_GLOB;

    /* expand argument parts */
    switch(subarg->id) {
      /* arithmetic substitution */
      case N_ARGARITH:
        n = expand_arith(&subarg->nargarith, nptr, lflags);
        break;

      /* parameter substitution */
      case N_ARGPARAM:
        n = expand_param(&subarg->nargparam, nptr, lflags);
        break;

      /* command substitution */
      case N_ARGCMD:
        n = expand_command(&subarg->nargcmd, nptr, lflags);
        break;

      /* constant string */
      default:
        n = expand_cat(subarg->nargstr.stra.s,
                       subarg->nargstr.stra.len, nptr, lflags);
        break;
    }
    
    if(n) nptr = &n;
  }
  
  return n;
}

