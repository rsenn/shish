#include "../tree.h"

/*  ----------------------------------------------------------------------- */
union node**
tree_append(union node** nptr, union node* list) {
  *nptr = list;

  while(*nptr) nptr = &(*nptr)->list.next;

  return nptr;
}
