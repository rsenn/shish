#include "../path_internal.h"

/* set out to the path_abs() of p[0..n) relative to the current directory; p may be out->s.
 * returns 1 on success, 0 on out of memory. */
int
path_abs_sa(stralloc* out, const char* p, size_t n) {
  stralloc cwd, tmp;
  size_t m;
  int ok = 0;

  stralloc_init(&cwd);
  stralloc_init(&tmp);
  path_getcwd(&cwd);
  m = path_abs(0, 0, p, n, cwd.s, cwd.len);

  if(stralloc_ready(&tmp, m + 1)) {
    tmp.len = path_abs(tmp.s, tmp.a, p, n, cwd.s, cwd.len);
    ok = stralloc_copy(out, &tmp) && stralloc_nul(out);
  }

  stralloc_free(&tmp);
  stralloc_free(&cwd);
  return ok;
}
