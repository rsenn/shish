#include "../tree.h"
#include <stdlib.h>

/* allocate a tree node and link
 * ----------------------------------------------------------------------- */
union node*
tree_newlink(union node** nptr, enum kind nod) {
  union node* n = tree_newnode(nod);

  if(*nptr == NULL)
    *nptr = n;
  else
    (*nptr)->next = n;

  return n;
}
