#include <assert.h>
#include "../expand.h"

node_t*
expand_getorcreate(node_t** out) {
  assert(out);
  if(!*out)
    *out = tree_newnode(N_ARG);
  return *out;
}
