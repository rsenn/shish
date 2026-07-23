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

        /* an empty literal chunk contributes zero bytes, but the
           X_LITERAL flag OR's cumulatively onto the shared argument
           node (expand_cat() keeps appending into the same node
           across subarg parts) -- setting it here regardless would
           taint an adjacent, still-to-come parameter/command
           substitution chunk that never went through the doubling
           expand_unescape() is meant to undo. Parser code routinely
           emits exactly such an empty N_ARGSTR immediately before a
           substitution (e.g. "$x" opens with a zero-length literal
           flush), so this isn't a rare edge case.

           A here-document body chunk (S_HEREDOC) is the same story
           for a different reason: parse_here.c's underlying
           parse_squoted()/parse_dquoted() calls skip the doubling
           entirely for P_HERE content, so it's already final too, and
           X_LITERAL must stay off for it as well (heredoc-body-loses-
           escaping, fixes/71). */
        n = expand_cat(subarg->nargstr.stra.s,
                       subarg->nargstr.stra.len,
                       nptr,
                       (subarg->nargstr.stra.len && !(subarg->nargstr.flag & S_HEREDOC))
                           ? (lflags | X_LITERAL)
                           : lflags);
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
