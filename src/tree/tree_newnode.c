#include "shell.h"
#include "byte.h"
#include "tree.h"

#ifndef DEBUG

/* allocate a tree node
 * ----------------------------------------------------------------------- */
union node *tree_newnode(enum nod_id nod) {
  union node *ret;

  if((ret = shell_alloc(sizeof(union node)))) {
    byte_zero(ret, sizeof(union node));
    ret->id = nod;
  }
  return ret;
}
#endif /* !defined(DEBUG) */