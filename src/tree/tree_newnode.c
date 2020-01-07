#include "byte.h"
#include "shell.h"
#include "tree.h"

#ifndef DEBUG_ALLOC

/* allocate a tree node
 * ----------------------------------------------------------------------- */
union node*
tree_newnode(enum nod_id nod) {
  union node* ret;

  if((ret = shell_alloc(sizeof(union node)))) {
    byte_zero(ret, sizeof(union node));
    ret->id = nod;
  }
  return ret;
}
#endif /* !defined(DEBUG_ALLOC) */