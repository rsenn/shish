#include "../filter.h"

int
filter_opt_count(const char* s, unsigned long* out) {
  unsigned long v = 0;

  if(!*s)
    return -1;

  for(; *s; s++) {
    if(*s < '0' || *s > '9' || v > (~0UL - (unsigned long)(*s - '0')) / 10)
      return -1;

    v = v * 10 + (unsigned long)(*s - '0');
  }

  *out = v;
  return 0;
}
