#include "walk_internal.h"
#include "../byte.h"
#include "../str.h"
#include <errno.h>

int
walk_open(struct walk* w, const char* root, unsigned flags) {
  size_t n = str_len(root);

  byte_zero(w, sizeof(*w));
  w->flags = flags;
  w->maxdepth = -1;
  w->next = WN_ROOT;
  w->pcap = n + 256;
  w->fcap = 16;
  /* child paths are root + '/' + name */
  w->rlen = n && root[n - 1] == '/' ? n : n + 1;

  arena_init(&w->pa, &arena_heap, w->pcap * 2);
  arena_init(&w->fa, &arena_heap, w->fcap * sizeof(*w->f) * 2);
  arena_init(&w->la, &arena_heap, 0);

  if((w->path = arena_alloc(&w->pa, w->pcap, 1)) && (w->f = arena_newn(&w->fa, struct walk_frame, w->fcap))) {
    byte_copy(w->path, n, root);
    w->plen = n;
    return 0;
  }

  walk_close(w);
  errno = ENOMEM;
  return -1;
}
