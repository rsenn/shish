#include "../tree.h"

/*  ----------------------------------------------------------------------- */
union node**
tree_append(union node** nptr, union node* list) {
  *nptr = list;

  while(*nptr) tree_skip(nptr);

  return nptr;
}
