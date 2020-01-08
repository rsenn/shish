#include "../scan.h"
#include <ctype.h>

size_t
scan_nonwhitenskip(const char* s, size_t limit) {
  const char* t = s;
  const char* u = t + limit;
  while(t < u && !isspace(*t)) ++t;
  return (size_t)(t - s);
}
