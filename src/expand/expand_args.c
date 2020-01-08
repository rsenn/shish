#include "expand.h"
#include "tree.h"

/* expand all arguments of an argument list
 * returns count of argument nodes
 * ----------------------------------------------------------------------- */
int
expand_args(union node* args, node_t** out) {
  expand_input* arg;
  node_t* n;
  int ret = 0;
  struct expand x;

  expand_init(&x, 0);

  //  n = expand_getorcreate(out);

  //  *nptr = NULL;

  for(arg = args; arg; arg = arg->narg.next) {

    if(!expand_arg(&x, &arg->narg))
      return -1;

    ret++;

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
      x.ptr = expand_break(x.ptr);

      n = *x.ptr = tree_newnode(N_ARG);
      stralloc_init(&n->narg.stra);
    }
  }

  expand_to(&x, out);

  return ret;
}
