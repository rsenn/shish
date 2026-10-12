#include "../scan.h"

size_t
scan_noncharsetnskip(const char* s, const char* charset, size_t limit) {
  const char  *t = s, *u = t + limit, *v;

  while(t < u) {
    for(v = charset; *v; ++v)
      if(*v == *t)
        break;

    if(*v == *t)
      break;

    ++t;
  }

  return (size_t)(t - s);
}
