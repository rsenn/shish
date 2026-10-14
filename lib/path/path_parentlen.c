#include "internal.h"

/* length of the parent of p[0..n), without the separators between them: always < n, or 0 when p has
 * no parent; the root is a parent. A loop over the ancestors, nearest first, ends by itself:
 *
 *   for(k = path_parentlen(p, n); k; k = path_parentlen(p, k)) rmdir(p[0..k)) ...
 *
 *   "a/b/c" -> 3    "a//b/" -> 1    "/a/b" -> 2    "/a" -> 1    "//a" -> 2
 *   "a", "a/", "/", "" -> 0
 * ----------------------------------------------------------------------- */
size_t
path_parentlen(const char* p, size_t n) {
  size_t root = path_rootlen(p, n), m = n - RSEPLEN(p, n), k;

  if(m <= root)
    return 0;

  m -= RCOMLEN(p, m);

  if(m <= root)
    return root;

  k = RSEPLEN(p, m);
  return m - k < root ? root : m - k;
}
