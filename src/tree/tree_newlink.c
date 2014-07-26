#include <stdlib.h>
#include "tree.h"

/* allocate a tree node and link
 * ----------------------------------------------------------------------- */
union node *tree_newlink(union node **nptr, enum nod_id nod)
{
  union node *n = tree_newnode(nod);
        
  if(*nptr == NULL)
    *nptr = n;
  else
    (*nptr)->list.next = n;
      
  return n;
}

