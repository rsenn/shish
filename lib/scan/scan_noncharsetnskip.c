#include "../scan.h"
#include <ctype.h>

size_t
scan_noncharsetnskip(const char* s, const char* charset, size_t limit) {
  const char* t = s;
  const char* u = t + limit;
  const char* i;
  while(t < u) {
    for(i = charset; *i; ++i)
      if(*i == *t)
        break;
    if(*i == *t)
      break;
    ++t;
  }
  return (size_t)(t - s);
}
