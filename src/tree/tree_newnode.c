#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../../lib/shell.h"
#include "../tree.h"

#ifndef DEBUG_ALLOC

/* allocate a tree node
 * ----------------------------------------------------------------------- */
union node*
tree_newnode(enum kind id) {
  union node* ret;
  size_t size = tree_nodesizes[id];

  if((ret = alloc(size))) {
    byte_zero(ret, size);
    ret->id = id;
  }
  return ret;
}
#endif /* !defined(DEBUG_ALLOC) */
