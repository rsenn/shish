#include "../scan.h"

size_t
scan_line(const char* s, size_t limit) {
  const char *t, *u;
  u = s + limit;
  for(t = s; t < u; ++t) {
    if(*t == '\n' || *t == '\r')
      break;
  }
  return (size_t)(t - s);
}
