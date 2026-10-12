#include "../path_internal.h"

/* "/x" is absolute; on Windows so is "\x" and "C:\x" ("C:" and "C:x" stay relative)
 *
 *   "some/dir"   -> 0
 *   "/some/dir"  -> 1
 *   "//some/dir" -> 2
 *   "c:/windows" -> 3
 *   "c:system32" -> 2
 */
size_t
path_rootlen(const char* x, size_t n) {
  if(path_isdrive(x, n))
    return 2 + path_rootlen(x + 2, n - 2);

  return n > 0 ? path_seplen(x, n) : 0;
}
