#include "../scan.h"

/* Return the number of trailing chars in s ending at limit that are not
   in charset */
size_t
scan_noncharsetnrskip(const char* s, const char* charset, size_t limit) {
  const char  *u = s + limit,*v;
  size_t count = 0;

  while(count < limit) {
    --u;
    for(v = charset; *v; ++v)
      if(*v == *u)
        break;

    if(*v)
      break;

    ++count;
  }

  return count;
}