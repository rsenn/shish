#include "../../lib/byte.h"
#include "../../lib/shell.h"
#include "../tree.h"

#ifdef DEBUG_ALLOC

/* allocate a tree node
 * ----------------------------------------------------------------------- */
union node*
tree_newnodedebug(const char* file, unsigned int line, enum kind id) {
  union node* ret;
  size_t size = tree_nodesizes[id];

  if((ret = shell_allocdebug(file, line, size))) {
    byte_zero(ret, size);
    ret->id = id;
  }
  return ret;
}
#endif /* defined(DEBUG_ALLOC) */
