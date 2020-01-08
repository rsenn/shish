#include "../expand.h"

void
expand_init(struct expand* ex, int flags) {
  ex->root = 0;
  ex->ptr = &ex->root;
  ex->flags = flags;
}