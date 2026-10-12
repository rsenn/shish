#include "../scan.h"
#include <ctype.h>

size_t
scan_nonwhitenskip(const char* s, size_t limit) {
  const char *t = s, *u = s + limit;

  while(t < u && !isspace(*t))
    ++t;

  return (size_t)(t - s);
}
