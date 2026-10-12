#include "../scan.h"
#include <ctype.h>

/* Return the number of leading chars in s that are in charset, but no
 * more than limit */
size_t
scan_charsetnskip(const char* s, const char* charset, size_t limit) {
  const char  *t = s, *u = s + limit, *v;

  while(t < u) {
    for(v = charset; *v; ++v)
      if(*v == *t)
        break;

    if(*v != *t)
      break;

    ++t;
  }

  return (size_t)(t - s);
}
