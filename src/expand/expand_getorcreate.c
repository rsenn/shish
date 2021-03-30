#include "../expand.h"
#include <assert.h>

union node*
expand_getorcreate(union node** out) {
  assert(out);

  if(!*out)
    *out = tree_newnode(N_ARG);

  return *out;
}
