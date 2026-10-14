#include "../path.h"
#include "../scan.h"

/* length of the first element of the list l[0..n): the bytes before a PATHLISTSEP_CHARS
 * separator (";" on Windows, ":" or ";" elsewhere), or n if there is none.
 *
 *   "a:b" -> 1    ":b" -> 0    "" -> 0
 * ----------------------------------------------------------------------- */
size_t
path_listlen(const char* l, size_t n) {
  return scan_noncharsetnskip(l, PATHLISTSEP_CHARS, n);
}
