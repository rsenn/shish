#include "expand.h"
#include "tree.h"

/* expand all arguments of an argument list
 * returns count of argument nodes
 * ----------------------------------------------------------------------- */
int
expand_args(union node* args, node_t** out) {
  expand_input* arg;
  expand_output* n;
  int ret = 0;
  struct expand x;

  n = expand_getorcreate(out);

  expand_init(&x, 0);
  expand_to(&x, out);

  //  *nptr = NULL;

  for(arg = args; arg; arg = arg->narg.next) {

    if((n = expand_arg(&x, &arg->narg))) {
      //      x.ptr = &n;
      ret++;
    }

    if(n == NULL)
      continue;

    if(n->narg.flag & X_GLOB) {
      if((n = expand_glob(x.ptr, n->narg.flag & ~X_GLOB))) {
        x.ptr = &n;
        ret++;
      }
    } else {
      expand_unescape(&n->narg.stra);
      n->narg.flag &= ~X_GLOB;
    }

    if(arg->list.next) {
      if(*x.ptr)
        x.ptr = &(*x.ptr)->narg.next;

      n = *x.ptr = tree_newnode(N_ARG);
      stralloc_init(&n->narg.stra);
    }
  }

  return ret;
}
