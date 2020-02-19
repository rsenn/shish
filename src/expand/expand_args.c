#include "../expand.h"
#include "../tree.h"
#include "../debug.h"
#include "../fd.h"

/* expand all arguments of an argument list
 * returns count of argument nodes
 * ----------------------------------------------------------------------- */
int
expand_args(union node* args, union node** nptr, int flags) {
  union node* arg;
  union node* n;
  int ret = 0;

  *nptr = NULL;

  for(arg = args; arg; arg = arg->narg.next) {

    debug_node(arg, 0);
    buffer_putnlflush(fd_err->w);

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
    } else {
      expand_unescape(&n->narg.stra);
      n->narg.flag &= ~X_GLOB;
    }

    if(arg->list.next) {
      n->list.next = tree_newnode(N_ARG);
      n = n->list.next;
      stralloc_init(&n->narg.stra);
      stralloc_nul(&n->narg.stra);
      ret++;
    }
  }

  return ret;
}
