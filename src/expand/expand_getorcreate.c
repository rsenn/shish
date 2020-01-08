#include "../expand.h"
#include <assert.h>

node_t*
expand_getorcreate(node_t** out) {
  assert(out);
  if(!*out)
    *out = tree_newnode(N_ARG);
  return *out;
}
