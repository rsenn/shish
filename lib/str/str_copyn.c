#include "../str.h"

size_t
str_copyn(void* out, const void* in, size_t max) {
  char* s = out;
  const char* t = in;
  size_t len = 0;

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
