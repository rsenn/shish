#include "walk_internal.h"

void
walk_skip(struct walk* w) {
  if(w->next == WN_DESCEND)
    w->next = WN_LIST;
}
