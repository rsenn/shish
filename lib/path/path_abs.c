#include "internal.h"

/* path_abs writes the canonical absolute form of p, relative to cwd, into out and returns its length.
 * Nothing is written unless cap >= cwdn + n + 3, otherwise cwdn + n + 2 (the bound) is returned.
 *
 *   "x/../y","/a/b" -> "/a/b/y"   "/z","/a/b" -> "/z"   ".","/a/b" -> "/a/b"
 * ----------------------------------------------------------------------- */
size_t
path_abs(char* out, size_t cap, const char* p, size_t n, const char* cwd, size_t cwdn) {
  size_t m = cwdn + n + 2;

  if(cap < m + 1)
    return m;

  m = path_cat(out, cap, cwd, cwdn, p, n);
  return path_canon(out, cap, out, m);
}
