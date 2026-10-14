#include "internal.h"

/* 1 if p[0..n) has no separator, i.e. it is a bare command name to search PATH for, 0 if it is a path
 *
 *   "ls" -> 1   "./ls" -> 0   "a/b" -> 0   "" -> 1
 * ----------------------------------------------------------------------- */
int
path_isname(const char* p, size_t n) {
  return COMLEN(p, n) == n;
}
