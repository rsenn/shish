#include "internal.h"

/* the absolute canonical form of p[0..n) in sa (NUL-terminated, replaced, not appended to).
 * A relative p is resolved against cwd[0..cwdn), or against the current directory if cwd is NULL or
 * cwdn is 0. p, cwd and sa->s must not overlap.
 *
 *   symbolic != 0  purely lexical, symlinks are kept (cd -L, realpath -s): no allocation besides sa
 *   symbolic == 0  every symlink is followed (cd -P, realpath); a component that does not exist is kept
 *
 * returns 0 with errno set (ELOOP after 40 links), otherwise 1 + the number of symlinks followed.
 *
 *   "x/../y","/a" -> "/a/y"   "/usr/lib/../include/" -> "/usr/include"   "//tmp" -> "/tmp"
 * ----------------------------------------------------------------------- */
int
path_resolve(const char* p, size_t n, stralloc* sa, int symbolic, const char* cwd, size_t cwdn) {
  stralloc own, full;
  int ret = 0;

  stralloc_init(&own);
  stralloc_init(&full);

  if(ISREL(p, n) && !DRIVE(p, n)) {
    if(!cwd || !cwdn) {
      if(!path_getcwd(&own))
        goto end;

      cwd = own.s;
      cwdn = own.len;
    }
  } else {
    cwd = 0;
    cwdn = 0;
  }

  if(symbolic) {
    if(!stralloc_ready(sa, cwdn + n + 3))
      goto end;

    sa->len = path_abs(sa->s, sa->a, p, n, cwd, cwdn);
    ret = 1;
  } else {
    if(!path_cat_sa(&full, cwd, cwdn, p, n))
      goto end;

    ret = path_canonicalize(full.s, sa, 0);
  }

end:
  stralloc_free(&own);
  stralloc_free(&full);
  return ret;
}
