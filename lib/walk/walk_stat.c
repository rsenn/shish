#include "walk_internal.h"

const struct stat*
walk_stat(struct walk* w) {
  int follow = (w->flags & WALK_LOGICAL) || ((w->flags & WALK_COMFOLLOW) && !w->e.depth);

  if(!w->hasst) {
    if((follow ? stat(w->path, &w->st) : walk_lstat(w->path, &w->st)) == -1)
      return NULL;
    w->hasst = 1;
  }
  return &w->st;
}
