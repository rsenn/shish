#include "expand.h"
#include "str.h"
#include "tree.h"
#include "var.h"
#include <stdlib.h>

/* concatenate <len> bytes from <b> to the argument list pointed to by <ex->ptr>
 * ----------------------------------------------------------------------- */
union node*
expand_cat(struct expand* ex, const char* b, unsigned int len) {
  union node* n = *ex->ptr;
  const char* ifs = NULL;
  unsigned int i, x;

  /* if we're not splitting create a new node if there isn't any, even if
     the stralloc has zero length, and concatenate the stralloc as a whole */
  if(ex->flags & (X_NOSPLIT | X_QUOTED)) {
    if(n == NULL) {
      n = *ex->ptr = tree_newnode(N_ARG);
      stralloc_zero(&n->narg.stra);
    }

    n->narg.flag |= ex->flags;
    stralloc_catb(&n->narg.stra, b, len);

    return n;
  }

  ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);

  for(i = 0;;) {
    /* skip field separators */
    for(x = 0; i < len; i++, x++) {
      if(ifs[str_chr(ifs, b[i])] == '\0')
        break;
    }

    /* finished */
    if(i == len)
      break;

    /* if there isn't already a node create one now! */
    if(n == NULL) {
      *ex->ptr = n = tree_newnode(N_ARG);
      ex->ptr = &n;
      stralloc_init(&n->narg.stra);
    }
    /* if there were separators delimit the
     current field by creating a new node */
    else if(x) {
      stralloc_nul(&n->narg.stra);

      if(ex->flags & X_GLOB) {
        if((n = expand_glob(ex->ptr, ex->flags & ~X_GLOB)))
          ex->ptr = &n;
      } else {
        expand_unescape(&n->narg.stra);
        n->narg.flag &= ~X_GLOB;
      }

      n->list.next = tree_newnode(N_ARG);
      n = n->list.next;
      stralloc_init(&n->narg.stra);
    }

    /* skip non-separators */
    for(x = 0; i < len; i++, x++) {
      if(ifs[str_chr(ifs, b[i])])
        break;
    }

    /* there were non-separators: fill the
     stralloc of the current argument node */
    /*      if(ex->flags & X_ESCAPE)
     expand_escape(&n->narg.stra, &b[i - x], x);
     else*/
    n->narg.flag |= ex->flags;
    stralloc_catb(&n->narg.stra, &b[i - x], x);

    /* finished */
    if(i == len)
      break;
  }

  return n;
}
