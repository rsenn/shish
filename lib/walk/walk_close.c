#include "walk_internal.h"

void
walk_close(struct walk* w) {
  arena_free(&w->la);
  arena_free(&w->pa);
  arena_free(&w->fa);
  w->next = WN_DONE;
}
