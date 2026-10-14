#include "internal.h"

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

  if(n && SEP(*p) && SEPLEN(p, n) == n) {
    if(len)
      *len = 1;

    return p;
  }

  end -= RSEPLEN(p, end);
  start = end - RCOMLEN(p, end);

  if(len)
    *len = end - start;

  return p + start;
}
