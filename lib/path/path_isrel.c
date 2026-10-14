#include "internal.h"

/* 1 if p[0..n) is not absolute (see path_isabs); the empty path is relative
 *
 *   "a/b" -> 1   "" -> 1   "/a" -> 0
 * ----------------------------------------------------------------------- */
int
path_isrel(const char* p, size_t n) {
  return ISREL(p, n);
}
