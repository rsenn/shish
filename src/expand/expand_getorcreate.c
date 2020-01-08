#include "../expand.h"

node_t*
expand_getorcreate(node_t** out) {
  if(!*out) {
    *out = tree_newnode(N_ARG);
    assert(*out);
  }
  return *out;
}
