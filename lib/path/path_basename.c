#include "../path_internal.h"

/* last component of p[0..n), POSIX basename(1) rules, as a view; *len (if not NULL) gets its length
 *
 *   "/usr/lib"  -> "lib"
 *   "/usr/"     -> "usr"     trailing separators are not part of it
 *   "//"        -> "/"       only separators: the first one
 *   ""          -> ""
 * ----------------------------------------------------------------------- */
const char*
path_basename(const char* p, size_t n, size_t* len) {
  size_t end = n, start;

  if(n && path_issep(*p) && path_seplen(p, n) == n) {
    if(len)
      *len = 1;

    return p;
  }

  end -= path_rseplen(p, end);
  start = end - path_rcomlen(p, end);

  if(len)
    *len = end - start;

  return p + start;
}
