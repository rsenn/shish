#include "../../lib/uint64.h"
#include "../expand.h"
#include "../tree.h"
#include "../debug.h"
#include <stdlib.h>
#include <assert.h>

#define max(a, b) ((a) >= (b) ? (a) : (b))
#define min(a, b) ((a) <= (b) ? (a) : (b))
extern int sh_no_position;

/* expand all parts of an N_ARG node
 * ----------------------------------------------------------------------- */
union node*
expand_arg(union node* node, union node** nptr, int flags) {
  union node *n = *nptr, *subarg;
  int i = 0;

  /* if(node) {
     debug_s("arg ");
     debug_node(node, 1);
     debug_newline(0);
     debug_fl();
   }*/

  /* loop through all parts of the word */
  for(subarg = (node && node->id == N_ARG) ? node->narg.list : node; subarg;
      subarg = subarg->next) {
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
      case N_ARGARITH: {
        n = expand_arith(&subarg->nargarith, nptr);
        break;
      }
      /* parameter substitution */
      case N_ARGPARAM: {
        n = expand_param(&subarg->nargparam, nptr, lflags);
        break;
      }
      /* command substitution */
      case N_ARGCMD: {
        n = expand_command(&subarg->nargcmd, nptr, lflags);
        break;
      }
      /* constant string */
      case N_ARGSTR: {
        assert(subarg->nargstr.stra.s);
        n = expand_cat(subarg->nargstr.stra.s,
                       subarg->nargstr.stra.len,
                       nptr,
                       lflags);
        break;
      }
      default: {
        /*debug_node(subarg, 0);
          debug_nl_fl();*/
        break;
      }
    }

    if(n == 0)
      break;

    /* debug_s("sub args #");
     debug_n(i);
     debug_s("  ");
     debug_node(subarg, 1);
     debug_newline(0);
     debug_fl();*/
    i++;
    nptr = &n;
  }

  /*  i = 0;
    for(subarg = *start; subarg; subarg = subarg->next) {
      debug_s("sub arg  #");
      debug_n(i);
      debug_s(" ");
      debug_node(subarg, 1);
      debug_newline(0);
      debug_fl();
      i++;
    }

    if(*start) {
      debug_s("expanded arg ");
      debug_node(*start, 1);
      debug_newline(0);
      debug_fl();
    }
  */
  return n;
}
