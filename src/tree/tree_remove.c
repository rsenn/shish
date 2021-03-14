#include "../tree.h"

/*  ----------------------------------------------------------------------- */
void
tree_remove(union node** nptr) {
  union node* node;
  if((node = *nptr)))
    tree_free(node);
  *nptr = 0;
}
