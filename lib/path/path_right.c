#include "../path_internal.h"

/* length of the directory part of s[0..n), as dirname(1) sees it
 *
 *   "a/b/c" -> 3    "a//b/" -> 1    "/a" -> 1 (the root)
 *   "a"     -> 0    "/"     -> 1    ""   -> 0
 * ----------------------------------------------------------------------- */
size_t
path_right(const char* s, size_t n) {
  while(n > 0 && path_issep(s[n - 1]))
    --n;

  if(n == 0)
    return path_issep(*s) ? 1 : 0;

  while(n > 0 && !path_issep(s[n - 1]))
    --n;

  if(n == 0)
    return 0;

  while(n > 0 && path_issep(s[n - 1]))
    --n;

  return n ? n : 1;
}
