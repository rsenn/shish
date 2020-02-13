#include "../str.h"

size_t
str_copyn(void* p1, const void* p2, size_t max) {
  size_t len;
  char* s;
  const char* t;
  s = p1;
  t = p2;
  len = 0;
  while(max-- > 0) {
    if(!(*s = *t))
      return len;
    ++s;
    ++t;
    ++len;
  }
  *s = '\0';
  return len;
}
