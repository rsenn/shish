#include "../../lib/uint64.h"
#include "../expand.h"
#include "../tree.h"
#include "../debug.h"
#include "../../lib/str.h"
#include <stdlib.h>
#include <assert.h>

#define max(a, b) ((a) >= (b) ? (a) : (b))
#define min(a, b) ((a) <= (b) ? (a) : (b))
extern int sh_no_position;

/* may the unquoted expansion results of this word be pathname-expanded as they are? Not when
   the word also holds quoted text that could carry a pattern character ("*"$x, "$a"$b), or a
   ${x+word} whose word has quoting of its own.
 * ----------------------------------------------------------------------- */
static int
expand_globres_ok(union node* node) {
  union node* sub;

  for(sub = (node && node->id == N_ARG) ? node->narg.list : node; sub; sub = sub->next) {
    if(sub->nargstr.flag & S_TABLE) {
      if(sub->id != N_ARGSTR)
        return 0;

      if(sub->nargstr.stra.len == 0)
        continue;

      if(str_chr(sub->nargstr.stra.s, '*') < sub->nargstr.stra.len || str_chr(sub->nargstr.stra.s, '?') < sub->nargstr.stra.len ||
         str_chr(sub->nargstr.stra.s, '[') < sub->nargstr.stra.len)
        return 0;

      continue;
    }

    if(sub->id == N_ARGPARAM && sub->nargparam.word)
      return 0;
  }

  return 1;
}

/* expand all parts of an N_ARG node
 * ----------------------------------------------------------------------- */
union node*
expand_arg(union node* node, union node** nptr, int flags) {
  union node *n = *nptr, *subarg;
  int i = 0, globres = expand_globres_ok(node);

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

    /* the result of an unquoted $x, $(cmd) or $((n)) is subject to pathname expansion */
    if(globres && !(lflags & X_QUOTED) && (subarg->id == N_ARGPARAM || subarg->id == N_ARGCMD || subarg->id == N_ARGARITH))
      lflags |= X_GLOBRES;

    /* expand argument parts */
    switch(subarg->id) {
      /* arithmetic substitution */
      case N_ARGARITH: {
        n = expand_arith(&subarg->nargarith, nptr, lflags);
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

        /* X_LITERAL must stay off for two cases, since it ORs
           cumulatively onto the shared argument node and would taint
           an adjacent chunk that doesn't need unescaping:
           - an empty literal chunk (parser routinely emits one right
             before a substitution, e.g. "$x" opens with one)
           - a here-document body chunk (S_HEREDOC): its underlying
             parse_squoted()/parse_dquoted() calls skip the doubling
             expand_unescape() would otherwise undo */
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
