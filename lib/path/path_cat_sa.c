#include "internal.h"

/* set out to the path_cat() of a and b; returns 1 on success, 0 on out of memory.
 *
 *   a == out->s  appends in place, no copy (the walker idiom: path_cat_sa(sa, sa->s, dirlen, name, nl))
 *   otherwise    builds the result aside first, so out may overlap a or b
 * ----------------------------------------------------------------------- */
int
path_cat_sa(stralloc* out, const char* a, size_t an, const char* b, size_t bn) {
  size_t n = path_cat(0, 0, a, an, b, bn);
  int ok;

  if(a == out->s && !(b >= out->s && b < out->s + out->a)) {
    if(!stralloc_ready(out, n + 1))
      return 0;

    path_cat(out->s, out->a, out->s, an, b, bn);
    out->len = n;
    return 1;
  } else {
    stralloc tmp;

    stralloc_init(&tmp);

    if(!stralloc_ready(&tmp, n + 1))
      return 0;

    path_cat(tmp.s, tmp.a, a, an, b, bn);
    tmp.len = n;
    ok = stralloc_copy(out, &tmp) && stralloc_nul(out);
    stralloc_free(&tmp);
    return ok;
  }
}
