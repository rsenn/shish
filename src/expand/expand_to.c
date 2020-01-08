#include "../expand.h"

void
expand_to(struct expand* ex, union node** out) {
  ex->root = *out;
  ex->ptr = out;
}