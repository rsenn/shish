#include "../expand.h"
#include <assert.h>

node_t**
expand_to(struct expand* ex, union node** out) {
  assert((*out) == NULL);
  assert(ex->root);
  *out = ex->root;

  do {
    out = &(*out)->narg.next;
  } while(*out);

  return out;
}