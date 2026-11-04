#include "walk_internal.h"
#include <errno.h>

const struct dirlist*
walk_kids(struct walk* w) {
  if(w->e.type != DL_DIR) {
    errno = ENOTDIR;
    return NULL;
  }

  if(!w->kids.s) {
    w->kpos = arena_tell(&w->la);
    w->kids = dirlist_read(&w->la, w->path, 0);

    if(!w->kids.s)
      return NULL;
  }

  return &w->kids;
}
