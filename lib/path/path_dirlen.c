#include "internal.h"

/* length of the directory part of s[0..n), as dirname(1) sees it
 *
 *   "a/b/c" -> 3
 *   "a//b/" -> 1
 *   "/a"    -> 1 (the root)
 *   "a"     -> 0
 *   "/"     -> 1
 *   ""      -> 0
 * ----------------------------------------------------------------------- */
size_t
path_dirlen(const char* s, size_t n) {
  if(n == 0)
    return 0;

  /* skip trailing path separators from the end */
  if((n -= RSEPLEN(s, n)) == 0)
    return 1;

  /* scan backwards past the current path component (non-separators) */
  if((n -= RCOMLEN(s, n)) == 0)
    return 0;

  /* skip backwards past any preceding consecutive separators */
  if((n -= RSEPLEN(s, n)) == 0)
    return 1;

  return n;
}