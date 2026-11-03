#include "../byte.h"
#include "../dirlist.h"
#include <errno.h>

static int
dirlist_strcmp(const char* a, const char* b) {
  return str_diff(a, b);
}

/* offsets o[] into s; compare the names at two of them */
#define LESS(i, j) (cmp(s + o[i] + 1, s + o[j] + 1) < 0)

static void
sift(const char* s, size_t* o, size_t i, size_t n, int (*cmp)(const char*, const char*)) {
  for(;;) {
    size_t c = 2 * i + 1, t;

    if(c >= n)
      return;
    if(c + 1 < n && LESS(c, c + 1))
      c++;
    if(!LESS(i, c))
      return;
    t = o[i], o[i] = o[c], o[c] = t;
    i = c;
  }
}

int
dirlist_sort(arena* a, struct dirlist* d, int (*cmp)(const char*, const char*)) {
  arena_pos pos;
  size_t *o, i, k = 0;
  char *copy, *p;

  if(d->n < 2)
    return 0;
  if(!cmp)
    cmp = dirlist_strcmp;

  pos = arena_tell(a);

  if(!(o = arena_allocn(a, sizeof(size_t), d->n, __alignof__(size_t))) || !(copy = arena_alloc(a, d->len, 1))) {
    arena_rewind(a, pos);
    errno = ENOMEM;
    return -1;
  }

  for(p = d->s, i = 0; i < d->n; i++, p = dirlist_next(p))
    o[i] = (size_t)(p - d->s);

  for(i = d->n / 2; i--;)
    sift(d->s, o, i, d->n, cmp);
  for(i = d->n; i-- > 1;) {
    size_t t = o[0];

    o[0] = o[i], o[i] = t;
    sift(d->s, o, 0, i, cmp);
  }

  for(i = 0; i < d->n; i++) {
    size_t l = 2 + str_len(d->s + o[i] + 1);

    byte_copy(copy + k, l, d->s + o[i]);
    k += l;
  }

  byte_copy(d->s, d->len, copy);
  arena_rewind(a, pos);
  return 0;
}
