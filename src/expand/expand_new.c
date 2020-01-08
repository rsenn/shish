#include "../expand.h"
#include <assert.h>

node_t*
expand_new(struct expand* ex) {

  if((*ex->ptr) == 0) {
    node_t** ptr = expand_getorcreate(ex->ptr);

    if(ex->root == 0 && ex->ptr == &ex->root)
      ex->root = *ptr;

    return *(ex->ptr = ptr);
  }
}
