#include "../expand.h"
#include "../tree.h"
#include "../debug.h"
#include "../fd.h"
#include "../parse.h"

/* expand all arguments of an argument list
 * returns count of argument nodes
 * ----------------------------------------------------------------------- */
int
expand_args(union node* args, union node** nptr, int flags) {
  union node* arg;
  union node* n;
  int ret = 0;

  *nptr = NULL;

  for(arg = args; arg; arg = arg->next) {

#ifdef DEBUG_OUTPUT_
    debug_node(arg, 0);
    debug_nl_fl();
#endif

    if((n = expand_arg(arg->narg.list, nptr, flags))) {
      nptr = &n;
      ret++;
    }

    if(n == NULL)
      continue;

    if(n->narg.flag & X_GLOB) {
      if((n = expand_glob(nptr, n->narg.flag & ~X_GLOB))) {
        nptr = &n;
        ret++;
      }
    } else if((n->narg.flag & X_LITERAL) && !(n->narg.flag & X_UNESCAPED)) {
      expand_unescape(&n->narg.stra, parse_isesc);
      n->narg.flag &= ~X_GLOB;
    } else {
      /* expand_unescape() nul-terminates as a side effect -- skipping
         it here (whether because there was no literal chunk at all,
         or because expand_cat() already unescaped every literal chunk
         itself -- X_UNESCAPED, fixes/70) must not also skip that, or
         ->stra.s stops being a valid C string and anything reading it
         as one (e.g. the "." builtin building a path from a command
         substitution result) walks off the end into whatever memory
         happens to follow the allocation. */
      stralloc_nul(&n->narg.stra);
    }

    if(arg->next) {
      n->next = tree_newnode(N_ARG);
      n = n->next;
      stralloc_init(&n->narg.stra);
      stralloc_nul(&n->narg.stra);
      ret++;
    }
  }

  return ret;
}
