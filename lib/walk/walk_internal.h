#ifndef WALK_INTERNAL_H
#define WALK_INTERNAL_H

#include "../walk.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#define walk_lstat(p, s) stat((p), (s))
#else
#define walk_lstat(p, s) lstat((p), (s))
#endif

/* what the next walk_read() does */
enum { WN_ROOT, WN_DESCEND, WN_LIST, WN_DONE };

/* one directory being listed */
struct walk_frame {
  char* cur;       /* next record */
  char* end;       /* one past the last record */
  arena_pos pos;   /* la before the list was read */
  size_t plen;     /* length of the directory's path */
  size_t nameoff;  /* offset of its last component */
  dev_t dev;
  ino_t ino;
  unsigned islink : 1;
  unsigned kept : 1;
};

/* grow the block p (oldsize bytes) that is all of arena *a to newsize: in
 * place if it fits, else into a bigger arena that replaces *a. NULL on
 * ENOMEM, *a and p unchanged. */
void* walk_grow(arena* a, void* p, size_t oldsize, size_t newsize);

#endif
