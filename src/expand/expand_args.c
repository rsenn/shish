#include "expand.h"
#include "tree.h"

/* expand all arguments of an argument list
 * returns count of argument nodes
 * ----------------------------------------------------------------------- */
int
expand_args(union node* args, union node** nptr) {
  union node* arg;
  union node* n;
  int ret = 0;
  struct expand x = EXPAND_INIT(0, nptr, 0);
  //  *nptr = NULL;

  for(arg = args; arg; arg = arg->list.next) {

    if((n = expand_arg(&x, &arg->narg))) {
      x.ptr = &n;
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
      n = EXPAND_ADDNODE(x.ptr);

      stralloc_init(&n->narg.stra);
    }
  }

  return ret;
}
