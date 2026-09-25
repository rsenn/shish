#include "../str.h"
#include "../byte.h"
#include "../alloc.h"

char*
str_ndup(const char* s, size_t n) {
  char* r;

  if(!(r = alloc(n + 1)))
    return r;

  byte_copy(r, n, s);
  r[n] = '\0';
  return r;
}
