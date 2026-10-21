#include "internal.h"

/* Returns the last component of p[0..n). */
const char*
path_basename(const char* p, size_t n, size_t* len) {
  size_t end = n, start;

  if(n && SEP(*p) && SEPLEN(p, n) == n) {
    if(len)
      *len = 1;

    return p;
  }

  end -= RSEPLEN(p, end);
  start = end - RCOMLEN(p, end);

  if(len)
    *len = end - start;

  return p + start;
}
