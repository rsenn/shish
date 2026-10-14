#include "../path.h"

/* start of the element after the one at pos, or (size_t)-1 when pos was the last.
 * A trailing separator leaves one more, empty, element.
 *
 *   "a:b" pos 0 -> 2    "a:b" pos 2 -> -1    "a:" pos 0 -> 2 (then "" at 2)
 * ----------------------------------------------------------------------- */
size_t
path_listnext(const char* l, size_t n, size_t pos) {
  size_t e = pos + path_listlen(l + pos, n - pos);

  return e < n ? e + 1 : (size_t)-1;
}
