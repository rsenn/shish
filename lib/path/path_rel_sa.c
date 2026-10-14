#include "../path_internal.h"

/* set out to the path_rel() of p from base; returns 1 on success.
 * returns 0 on out of memory, or when no relative path exists (out is then a copy of p). */
int
path_rel_sa(stralloc* out, const char* p, size_t pn, const char* base, size_t bn) {
  size_t n = path_rel(0, 0, p, pn, base, bn);
  stralloc tmp;
  int ok;

  if(n == (size_t)-1) {
    stralloc_copyb(out, p, pn);
    stralloc_nul(out);
    return 0;
  }

  stralloc_init(&tmp);

  if(!stralloc_ready(&tmp, n + 1))
    return 0;

  path_rel(tmp.s, tmp.a, p, pn, base, bn);
  tmp.len = n;
  ok = stralloc_copy(out, &tmp) && stralloc_nul(out);
  stralloc_free(&tmp);
  return ok;
}
