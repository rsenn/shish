#include "dirlist_internal.h"
#include "../byte.h"
#include <errno.h>

int
dirlist_add(arena* a, struct dirlist_buf* b, int type, const char* name, size_t nlen) {
  size_t need = b->d.len + nlen + 2;

  if(need > b->cap) {
    size_t cap = b->cap * 2 > need ? b->cap * 2 : need;
    char* s;

    /* in place while the blob is the newest allocation with room after it,
       else a fresh block and a copy (the old one stays as slack) */
    if(!(s = arena_grow(a, b->d.s, b->cap, cap))) {
      if(!(s = arena_alloc(a, cap, 1))) {
        errno = ENOMEM;
        return -1;
      }
      byte_copy(s, b->d.len, b->d.s);
    }
    b->d.s = s;
    b->cap = cap;
  }

  b->d.s[b->d.len] = (char)type;
  byte_copy(b->d.s + b->d.len + 1, nlen, name);
  b->d.s[b->d.len + 1 + nlen] = '\0';
  b->d.len = need;
  b->d.n++;
  return 0;
}
