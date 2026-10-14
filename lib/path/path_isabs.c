#include "internal.h"

/* 1 if p[0..n) is absolute: its root contains a separator; on Windows "C:/x" too, but not "C:" or "C:x"
 *
 *   "/"  -> 1   "//a" -> 1   "a/b" -> 0   "" -> 0   "./a" -> 0
 * ----------------------------------------------------------------------- */
int
path_isabs(const char* p, size_t n) {
  return ISABS(p, n);
}
