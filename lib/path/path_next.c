#include "internal.h"

/* end of the first component of p[0..n) that starts at or after pos, or 0 if none is left. The root
 * (see path_rootlen) is skipped, so a loop over the prefixes starts with the first name:
 *
 *   for(k = 0; (k = path_next(p, n, k)); ) mkdir(p[0..k)) ...
 *
 *   "/a//b/" : 0 -> 2 ("/a"), 2 -> 5 ("/a//b"), 5 -> 0
 *   "a"      : 0 -> 1, 1 -> 0
 *   "/", ""  : 0 -> 0
 * ----------------------------------------------------------------------- */
size_t
path_next(const char* p, size_t n, size_t pos) {
  size_t root = path_rootlen(p, n);

  if(pos < root)
    pos = root;

  if(pos > n)
    return 0;

  pos += SEPLEN(p + pos, n - pos);

  if(pos >= n)
    return 0;

  return pos + COMLEN(p + pos, n - pos);
}
